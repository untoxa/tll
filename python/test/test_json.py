#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: sts=4 sw=4 et

from tll.test_util import Accum
import tll.channel as C
from tll.chrono import *
from tll.error import TLLError

import decimal
import json
import pytest

SCHEME = '''yamls://
- name: sub
  fields:
    - {name: s0, type: int32}
    - {name: s1, type: 'double[4]'}
  options: {a: 1, b: 2}

- name: test
  id: 1
  fields:
    - {name: f0, type: int8, options: {a: 10, b: 20}}
    - {name: f1, type: int64}
    - {name: f2, type: double}
    - {name: f3, type: byte32, options.type: string}
    - {name: f4, type: '*int16'}
    - {name: f5, type: 'sub[4]'}
    - {name: f6, type: string}

- name: list_sub
  id: 10
  fields:
    - {name: s, type: string}
    - {name: f0, type: '*int16', options.json.expected-list-size: 4}
    - {name: f1, type: 'int32[4]'}

- name: list_test
  id: 11
  fields:
    - {name: f0, type: '**int16', options.json.expected-list-size: 4}
    - {name: f1, type: '*list_sub', options.json.expected-list-size: 4}
    - {name: f2, type: 'list_sub[4]'}

- name: list
  options.json.message-as-list: yes
  fields:
    - {name: s0, type: sub, options.json.inline-message: yes}

- name: enums
  id: 10
  enums:
    e1: {type: int8,  enum: {A: 1, B: 2}}
    e2: {type: int16, enum: {C: 1, D: 2}}
    e4: {type: int32, enum: {E: 1, F: 2}}
    e8: {type: int64, enum: {G: 1, H: 2}}
  fields:
    - {name: f0, type: e1, options.json.enum-as-int: yes}
    - {name: f1, type: e2}
    - {name: f2, type: e4}
    - {name: f3, type: e8}
'''

class xTest:
    def setup(self):
        self.output = Accum('json+direct://', name='out', scheme=SCHEME, **{'dump':'yes'})
        self.input = Accum('direct://', name='in', master=self.output)

        self.output.open()
        self.input.open()

    def teardown(self):
        self.input = None
        self.output = None

    def test_list_message(self):
        #m = self.output.scheme['test'].object(f0=10, f1=100, f2=10.1, f3=b"string")
        data = dict(f0=10, f1=100, f2=10.1, f3="string", f4=[11, 12, 13], f5=[{'s0':10, 's1':[10.1]}, {'s0':20, 's1':[20.1, 20.2]}], f6="ыыы")
        self.output.post(name='test', data=data, seq=1000)
        assert [(x.type, x.msgid) for x in self.input.result] == [(C.Type.Data, 1)]
        r = json.loads(self.input.result[0].data.tobytes())
        #assert r.pop('_ce_name') == 'test'
        #assert r.pop('_ce_seq') == 1000
        assert r == dict(_ce_name='test', _ce_seq=1000, **data)

        self.input.post(data = json.dumps(r, ensure_ascii=False).encode('utf-8'))

        assert [x.msgid for x in self.output.result if x.type == C.Type.Data] == [1]
        assert self.output.unpack(self.output.result[-1]).as_dict() == data

    def test_list_overflow(self):
        self.input.post(json.dumps({'_ce_name':'list_sub', 'f1':[float(i) for i in range(5)]}).encode('utf-8'))
        assert self.output.result == []

        self.input.post(json.dumps({'_ce_name':'list_test', 'f2':[{'s':str(i)} for i in range(5)]}).encode('utf-8'))
        assert self.output.result == []

    def _test_list_resize(self, msg, data):
        self.input.post(json.dumps(dict(_ce_name=msg, **data)).encode('utf-8'))

        assert self.output.result != []
        r = self.output.unpack(self.output.result[-1])
        print(r.as_dict())
        assert r.as_dict() == data
        self.output.result = []

    def test_list_resize(self):
        self._test_list_resize('list_test', {'f0':[list(range(i, i + 5)) for i in range(5)]})
        self._test_list_resize('list_test', {'f1':[{'f0':[i], 's': '{:d}'.format(i)} for i in range(10)]})

@pytest.mark.parametrize("t,v", [
    ('int8', 123),
    ('int16', 12323),
    ('int32', 123123),
    ('int64', 123123),
    ('uint8', 231),
    ('uint16', 53123),
    ('uint32', 123123),
    ('double', 123.123),
#    ('byte8', b'abcd\0\0\0\0'),
    ('byte8, options.type: string', 'abcd'),
    ('string', 'abcd'),
#    ('decimal128', (decimal.Decimal('1234567890.e-5'), '1234567890.e-5')),
#    ('int32, options.type: fixed3', decimal.Decimal('123.456')),
#    ('int32, options.type: duration, options.resolution: us', (Duration(123000, Resolution.us), '12300us')),
#    ('int64, options.type: time_point, options.resolution: s', (TimePoint(1609556645, Resolution.second), '2021-01-02T03:04:05')),
    ('"int32[4]"', [1, 2, 3]),
    ('"*int32"', [1, 2, 3]),
    ('"*string"', ['a', 'bc', 'def']),
    ('sub', {'s0': 10, 's1': 'string'})
])
def test_simple(t, v):
    if isinstance(v, tuple):
        v, jv = v
    else:
        jv = v
    scheme = f'''yamls://
- name: sub
  fields:
    - {{name: s0, type: int32 }}
    - {{name: s1, type: string }}
- name: msg
  id: 10
  fields:
    - {{name: g0, type: int64 }}
    - {{name: f0, type: {t} }}
    - {{name: g1, type: int64 }}
'''
    s = Accum('json+direct://', scheme=scheme, name='json', dump='scheme')
    c = Accum('direct://', name='raw', master=s, dump='text')
    s.open()
    c.open()

    s.post({'g0': -1, 'f0': v, 'g1': -1}, name='msg', seq=100)

    assert [(m.msgid, m.seq) for m in c.result] == [(10, 100)]
    data = json.loads(c.result[0].data.tobytes())
    assert data == {'_tll_name': 'msg', '_tll_seq': 100, 'f0': jv, 'g0': -1, 'g1': -1}

    c.post(json.dumps(data).encode('utf-8'))
    assert [(m.msgid, m.seq) for m in s.result] == [(10, 100)]
    assert s.unpack(s.result[0]).as_dict() == {'g0': -1, 'g1': -1, 'f0': v}
