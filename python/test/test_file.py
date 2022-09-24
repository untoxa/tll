#!/usr/bin/env python3
# vim: sts=4 sw=4 et

import collections
import os
import struct

import pytest

from tll.channel import Context
from tll.error import TLLError
from tll.test_util import Accum

META_SIZE = 4 + 12 + 24 # Size + frame + meta

@pytest.fixture
def context():
    return Context()

@pytest.fixture
def filename(tmp_path):
    return tmp_path / 'file.dat'

@pytest.fixture
def writer(context, filename):
    return context.Channel(f'file://{filename}', name='writer', dump='frame', dir='w', block='1kb')

@pytest.fixture
def reader(context, filename):
    return Accum(f'file://{filename}', name='reader', dump='frame', dir='r', context=context, autoclose='no')

Frame = collections.namedtuple('Frame', ('size', 'msgid', 'seq'))

def frame(data):
    return Frame(*struct.unpack('iiq', data[:16]))

def test_basic(writer, reader, filename):
    w = writer
    w.open()
    assert w.dcaps == w.DCaps.Zero

    with pytest.raises(TLLError): w.post(b'x' * 1024 * 1024)
    with pytest.raises(TLLError): w.post(b'x' * (1024 - 15))

    assert filename.stat().st_size == META_SIZE
    fp = filename.open('rb')

    w.post(b'a' * 128, seq=0, msgid=0)
    assert filename.stat().st_size == META_SIZE + (128 + 16) * 1

    data = fp.read(16)
    assert data == b'\x28\0\0\0Meta\0\0\0\0\0\0\0\0'

    data = fp.read(0x28 - 16) # Skip meta

    data = fp.read(128 + 16)
    assert frame(data) == Frame(128 + 16, 0, 0)
    assert data[16:] == b'a' * 128

    w.post(b'b' * 128, seq=1, msgid=10)
    assert filename.stat().st_size == META_SIZE + (128 + 16) * 2

    data = fp.read(128 + 16)
    assert frame(data) == Frame(128 + 16, 10, 1)
    assert data[16:] == b'b' * 128

    reader.open()
    assert reader.dcaps == reader.DCaps.Process | reader.DCaps.Pending

    reader.process()
    assert [(x.seq, x.msgid, len(x.data), x.data.tobytes()) for x in reader.result] == [(0, 0, 128, b'a' * 128)]
    reader.result = []

    reader.process()
    assert [(x.seq, x.msgid, len(x.data), x.data.tobytes()) for x in reader.result] == [(1, 10, 128, b'b' * 128)]
    reader.result = []

    reader.process()
    assert [(x.seq, x.msgid, len(x.data), x.data.tobytes()) for x in reader.result] == []
    assert reader.dcaps == reader.DCaps.Process

    w.post(b'c' * 128, seq=2, msgid=20)
    assert filename.stat().st_size == META_SIZE + (128 + 16) * 3

    data = fp.read(128 + 16)
    assert frame(data) == Frame(128 + 16, 20, 2)
    assert data[16:] == b'c' * 128

    reader.process()
    assert [(x.seq, x.msgid, len(x.data), x.data.tobytes()) for x in reader.result] == [(2, 20, 128, b'c' * 128)]
    reader.result = []

def test_open_error(reader, writer, filename):
    with pytest.raises(TLLError): reader.open()
    filename.mkdir()
    with pytest.raises(TLLError): writer.open()

def test_block_boundary(writer, reader, filename):
    w = writer
    w.open()

    assert filename.stat().st_size == META_SIZE
    fp = filename.open('rb')

    w.post(b'a' * 512, seq=0, msgid=0)
    assert filename.stat().st_size == META_SIZE + (512 + 16) * 1

    data = fp.read(16)
    assert data == b'\x28\0\0\0Meta\0\0\0\0\0\0\0\0'

    data = fp.read(0x28 - 16) # Skip meta

    data = fp.read(512 + 16)
    assert frame(data) == Frame(512 + 16, 0, 0)
    assert data[16:] == b'a' * 512

    reader.open()

    reader.process()
    assert [(x.seq, x.msgid, len(x.data), x.data.tobytes()) for x in reader.result] == [(0, 0, 512, b'a' * 512)]
    reader.result = []

    reader.process()
    assert [(x.seq, x.msgid, len(x.data), x.data.tobytes()) for x in reader.result] == []
    assert reader.dcaps == reader.DCaps.Process

    w.post(b'b' * 512, seq=1, msgid=10)
    assert filename.stat().st_size == 1024 + 4 + (512 + 16) * 1

    data = fp.read(16)
    assert frame(data) == Frame(-1, 0, 0)
    fp.seek(1024)

    data = fp.read(4)
    assert data == b'\x04\0\0\0'

    data = fp.read(512 + 16)
    assert frame(data) == Frame(512 + 16, 10, 1)
    assert data[16:] == b'b' * 512

    reader.process()
    assert [(x.seq, x.msgid, len(x.data), x.data.tobytes()) for x in reader.result] == [(1, 10, 512, b'b' * 512)]
    reader.result = []

@pytest.mark.parametrize("seq,r", [(None, 10), (0, 10), (5, 10), (100, 100), (105, 110)])
def test_open_seq(seq, r, writer, reader):
    writer.open()

    for i in range(100):
        writer.post(b'abc' * i, seq = 10 * (i + 1), msgid = i)

    reader.open(**({'seq': str(seq)} if seq is not None else {}))
    reader.process()
    assert [(x.seq, x.msgid, len(x.data)) for x in reader.result] == [(r, r // 10 - 1, 3 * (r // 10 - 1))]

def test_meta(context, filename):
    SCHEME = '''yamls://
- name: msg
  fields: [{name: f0, type: int32}]
'''
    w = context.Channel(f'file://{filename}', name='writer', dump='frame', dir='w', block='1kb', scheme=SCHEME)
    r = context.Channel(f'file://{filename}', name='reader', dump='frame', dir='r', context='context', block='4kb')

    w.open()
    assert os.path.exists(filename)
    r.open()
    assert r.scheme != None
    assert [m.name for m in r.scheme.messages] == ['msg']
    assert r.config.get('block', '') == '1kb'

def test_autoclose(context, filename, writer):
    writer.open()
    reader = Accum(f'file://{filename}', name='reader', dump='frame', dir='r', context=context, autoclose='yes')

    for i in range(10):
        writer.post(b'abc' * i, seq = 10 * (i + 1), msgid = i)

    reader.open(seq='50')
    reader.process()
    for _ in range(10):
        reader.process()
    assert [m.seq for m in reader.result] == list(range(50, 101, 10))
    assert reader.state == reader.State.Closed
