#!/usr/bin/env python3
# vim: sts=4 sw=4 et

from tll import asynctll, chrono
import tll.channel as C
from tll.channel.base import Base
from tll.channel.prefix import Prefix
from tll.channel.logic import Logic
from tll.config import Url
from tll.error import TLLError
from tll.test_util import Accum

import datetime
import pytest

@pytest.fixture
def context():
    ctx = C.Context()
    return ctx

class Echo(Base):
    PROTO = "echo"

    OPEN_POLICY = Base.OpenPolicy.Manual
    CLOSE_POLICY = Base.ClosePolicy.Long
    CHILD_POLICY = Base.ChildPolicy.Many
    POST_OPENING_POLICY = Base.PostPolicy.Enable
    POST_CLOSING_POLICY = Base.PostPolicy.Disable

    def _init(self, props, master=None):
        self._child = None
        self.scheme_control = self.context.scheme_load('yamls://[{name: Control, id: 10}]')

    def _open(self, props):
        self._child = self.context.Channel(self._child_url_parse('null://', 'child'))
        self._child_add(self._child)
        self._child_add(self.context.Channel('null://;name=orphan'))
        self.config_info['echo'] = 'yes'

    def _close(self, force=False):
        try:
            self._child_del(self._child)
        except:
            pass

    def _process(self, timeout, flags):
        if self.state == C.State.Opening:
            self.state = C.State.Active
        elif self.state == C.State.Closing:
            self.state = C.State.Closed

    def _post(self, msg, flags):
        self._callback(msg.copy())

class EchoV2(Echo):
    PROTO = "echo-v2"
    def _open(self, props):
        super()._open(props)
        self.config_info['echo'] = 'v2'

class OpenTest(Base):
    PROTO = "open-test"

    PROCESS_POLICY = Base.ProcessPolicy.Never

    def _open(self, props):
        self.log.info("Open: {}", props.as_dict())
        if props.as_dict() != {'a':'1', 'b':'2', 'c':'3'}:
            raise RuntimeError("Invalid open parameters")

class TestPrefix(Prefix):
    __test__ = False
    PROTO = "prefix+"

    def _init(self, url, master=None):
        super()._init(url, master)

    def _open(self, props):
        super()._open(props)

class TestLogic(Logic):
    __test__ = False
    PROTO = "logic"

    def _init(self, url, master=None):
        super()._init(url, master)

        if len(self._channels.get('input', [])) != 1:
            raise RuntimeError("Need exactly one input, got: {}".format([c.name for c in self._channels.get('input', [])]))
        self._input = self._channels['input'][0]
        if len(self._channels.get('output', [])) != 1:
            raise RuntimeError("Need exactly one output, got: {}".format([c.name for c in self._channels.get('output', [])]))
        self._input = self._channels['input'][0]
        self._output = self._channels['output'][0]

    def _open(self, props):
        super()._open(props)

    def _logic(self, channel, msg):
        if channel != self._input:
            return
        if msg.type != msg.Type.Data:
            return
        self._output.post(msg)

def test():
    ctx = C.Context()

    with pytest.raises(TLLError): ctx.Channel("echo://;name=echo")
    ctx.register(Echo)
    c = Accum("echo://;name=echo", context=ctx, dump='text', scheme='yamls://[{name: Data, id: 10}]')
    cfg = c.config

    with pytest.raises(RuntimeError): cfg['info.a'] = 'b'
    with pytest.raises(RuntimeError): cfg.unlink('url')

    pyc = C.channel_cast(c)
    assert isinstance(pyc, Echo)

    pyc = C.channel_cast(c, Echo)
    assert isinstance(pyc, Echo)

    with pytest.raises(TypeError): C.channel_cast(c, TestPrefix)

    assert c.state == c.State.Closed
    assert cfg.get("state", "") == "Closed"
    assert [x.name for x in c.children] == []

    with pytest.raises(TLLError): c.post(b'')
    assert cfg.get('info.echo', '') != 'yes'
    c.open()
    assert cfg['info.echo'] == 'yes'

    assert [x.name for x in c.children] == ['echo/child', 'orphan']

    with pytest.raises(TypeError): C.channel_cast(c.children[0])

    assert c.state == c.State.Opening
    assert cfg.get("state", "") == "Opening"

    c.process()

    assert c.state == c.State.Active
    assert cfg.get("state", "") == "Active"

    assert c.scheme != None
    assert [(m.name, m.msgid) for m in c.scheme.messages] == [('Data', 10)]

    assert c.scheme_control != None
    assert [(m.name, m.msgid) for m in c.scheme_control.messages] == [('Control', 10)]

    assert c.result == []
    now = datetime.datetime.now()
    c.post(b'xxx', seq=100, time=now)
    assert [(m.seq, m.data.tobytes()) for m in c.result] == [(100, b'xxx')]
    assert c.result[-1].time == chrono.TimePoint(now, resolution=chrono.Resolution.ns, type=int)

    c.close()
    assert [x.name for x in c.children] == ['orphan']
    assert c.state == c.State.Closing
    with pytest.raises(TLLError): c.post(b'')
    c.process()
    assert c.state == c.State.Closed
    del c

    assert ctx.get('orphan') == None

    ctx.unregister(Echo)
    with pytest.raises(TLLError): ctx.Channel("echo://;name=echo")

def test_open_params():
    ctx = C.Context()

    ctx.register(OpenTest)
    c = ctx.Channel("open-test://;name=open")

    with pytest.raises(TLLError): c.open()
    assert c.state == c.State.Error
    c.close()
    assert c.config.as_dict()['open'] == {}

    c.open('a=1;b=2;c=3')
    assert c.state == c.State.Active
    assert c.config.as_dict()['open'] == {'a': '1', 'b': '2', 'c': '3'}
    c.close()

    c.open('c=3;b=2;a=1')
    assert c.state == c.State.Active
    assert c.config.as_dict()['open'] == {'a': '1', 'b': '2', 'c': '3'}
    c.close()

    c.open('a=1;b=2', c='3')
    assert c.state == c.State.Active
    c.close()

    c.open({'a': '1', 'b': '2', 'c': '3'})
    assert c.state == c.State.Active
    c.close()

def test_prefix():
    ctx = C.Context()

    with pytest.raises(TLLError): ctx.Channel("prefix+null://;name=channel")
    ctx.register(Echo)
    ctx.register(TestPrefix)
    c = Accum("prefix+echo://;name=channel", context=ctx, dump='text', scheme='yamls://[{name: Data, id: 10}]')
    cfg = c.config

    pyc = C.channel_cast(c)
    assert isinstance(pyc, TestPrefix)

    assert c.state == c.State.Closed
    assert cfg.get("state", "") == "Closed"
    assert [x.name for x in c.children] == ['channel/prefix']

    c.open()

    assert [x.name for x in c.children] == ['channel/prefix']

    assert c.state == c.State.Opening
    assert cfg.get("state", "") == "Opening"

    c.process()

    assert c.state == c.State.Opening
    assert cfg.get("state", "") == "Opening"

    c.children[0].process()

    assert c.state == c.State.Active
    assert cfg.get("state", "") == "Active"
    assert cfg.get('python.info.echo') == 'yes'
    assert cfg.get('info.echo') == 'yes'

    assert c.scheme != None
    assert [(m.name, m.msgid) for m in c.scheme.messages] == [('Data', 10)]

    assert c.scheme_control != None
    assert [(m.name, m.msgid) for m in c.scheme_control.messages] == [('Control', 10)]

    assert c.result == []
    now = datetime.datetime.now()
    c.post(b'xxx', seq=100, time=now)
    assert [(m.seq, m.data.tobytes()) for m in c.result] == [(100, b'xxx')]
    #assert c.result[0].time == chrono.TimePoint(now, resolution=chrono.Resolution.ns, type=int)

    c.result = []
    c.post(b'zzz', seq=200, type=C.Type.Control, addr=0xbeef, time=now)
    assert [(m.type, m.seq, m.addr, m.data.tobytes()) for m in c.result] == [(C.Type.Control, 200, 0xbeef, b'zzz')]

    c.close()
    assert [x.name for x in c.children] == ['channel/prefix']
    del c

    ctx.unregister(TestPrefix)
    with pytest.raises(TLLError): ctx.Channel("prefix+null://;name=channel")

def test_alias():
    ctx = C.Context()

    ctx.register(Echo)
    ctx.register(TestPrefix)

    ctx.alias('aecho', 'echo://')
    ctx.alias('aprefix+', 'prefix+://')
    ctx.alias('alias', 'aprefix+echo://')

    c = ctx.Channel('aecho://;name=echo')
    assert str(Url(c.config.sub('init'))) == 'echo://;name=echo'
    assert str(Url(c.config.sub('url'))) == 'echo://;name=echo' # Compatibility

    c = ctx.Channel('aprefix+aecho://;name=prefix')
    assert str(Url(c.config.sub('init'))) == 'prefix+aecho://;name=prefix'

    c = ctx.Channel('alias://;name=alias')
    assert str(Url(c.config.sub('init'))) == 'prefix+echo://;name=alias'

def test_logic():
    ctx = C.Context()
    sl = ctx.stat_list

    with pytest.raises(TLLError): ctx.Channel("logic://;name=logic")
    ctx.register(TestLogic)

    with pytest.raises(TLLError): ctx.Channel("logic://;name=logic;tll.channel.input=input;tll.channel.output=input")

    i = ctx.Channel('mem://;name=input;dump=yes')
    o = Accum('mem://;name=output;dump=yes', master=i, context=ctx)
    l = ctx.Channel("logic://;name=logic;tll.channel.input=input;tll.channel.output=input;stat=yes")

    assert len(list(sl)) == 1
    assert [x.name for x in sl] == ['logic']
    fields = iter(sl).swap()
    assert [(f.name, f.value) for f in fields[:-1]] == [('rx', 0), ('rx', 0), ('tx', 0), ('tx', 0)]
    assert (fields[-1].name, fields[-1].count) == ('time', 0)

    l.open()

    i.open()
    o.open()

    o.post(b'xxx')

    assert [m.data.tobytes() for m in o.result] == []

    i.process()

    assert [m.data.tobytes() for m in o.result] == []

    o.process()

    assert [m.data.tobytes() for m in o.result] == [b'xxx']

    fields = iter(sl).swap()
    assert [(f.name, f.value) for f in fields[:-1]] == [('rx', 1), ('rx', 3), ('tx', 0), ('tx', 0)]
    assert (fields[-1].name, fields[-1].count) == ('time', 1)
    assert fields[-1].sum > 1000

class _test_base_logic:
    CHANNELS = {}
    SCHEME = None

    def setup_method(self):
        self.channels = {}
        self.ctx = C.Context()
        self.loop = asynctll.Loop(self.ctx)
        channels = set()
        for k,l in self.CHANNELS.items():
            if not l: continue
            for c in [x.strip() for x in l.split(',')]:
                if c == '':
                    raise ValueError("Empty channel name in '{}'", l)
                channels.add(c)
        kw = {}
        if self.SCHEME:
            kw['scheme'] = self.SCHEME
        for c in channels:
            self.channels['test/' + c] = self.loop.Channel('direct://;name=test/{name}'.format(name=c), **kw)
            self.channels[c] = self.loop.Channel('direct://;name={name};master=test/{name}'.format(name=c), **kw)

        for n,c in self.channels.items():
            if n.startswith('test/'):
                c.open()

    def teardown_method(self):
        for c in self.channels.values():
            c.close()
        self.channels = None
        self.loop = None
        self.ctx = None

def async_tests(cls):
    for k,v in list(cls.__dict__.items()):
        if not k.startswith('async_test') or not callable(v):
            continue
        print(f"Wrap {k}: {v}")
        def wrap(x):
            def f(obj, *a, **kw):
                future = x(obj, *a, **kw)
                if future is None:
                    return
                obj.loop.run(future)
            return f
        setattr(cls, 'test_async_' + k, wrap(v))
    return cls

@async_tests
class TestLogicAsync(_test_base_logic):
    CHANNELS = {'input': 'input', 'output': 'output'}
    SCHEME = 'yamls://[{name: msg, id: 10, fields: [{name: f0, type: int32}]}]'

    async def async_test(self):
        self.ctx.register(TestLogic)
        logic = self.loop.Channel('logic://;name=logic;tll.channel.input=input;tll.channel.output=output')
        logic.open()

        i, o = self.channels['test/input'], self.channels['test/output']

        self.channels['input'].open()
        self.channels['output'].open()

        msg = C.Message(msgid=10, data=i.scheme['msg'].object(f0=0xbeef).pack())

        i.post(msg, seq=10)
        i.post(b'zzz', seq=20)

        m = await o.recv()
        assert m.data.tobytes() == b'\xef\xbe\x00\x00'
        assert m.seq == 10

        m = await o.recv()
        assert m.data.tobytes() == b'zzz'
        assert m.seq == 20

def test_logic_async_func():
    ctx = C.Context()
    ctx.register(TestLogic)
    loop = asynctll.Loop(ctx)
    async def main(loop):
        i, o = loop.Channel('direct://;name=test/input'), loop.Channel('direct://;name=test/output')
        li, lo = loop.Channel('direct://;name=input;master=test/input'), loop.Channel('direct://;name=output;master=test/output')
        i.open(), o.open()
        li.open(), lo.open()

        logic = loop.Channel('logic://;name=logic;tll.channel.input=input;tll.channel.output=output')
        logic.open()

        i.post(b'xxx', seq=10)
        i.post(b'zzz', seq=20)

        m = await o.recv()
        assert m.data.tobytes() == b'xxx'
        assert m.seq == 10

        m = await o.recv()
        assert m.data.tobytes() == b'zzz'
        assert m.seq == 20

    loop.run(main(loop))

def test_stat():
    ctx = C.Context()
    ctx.register(Echo)

    l = ctx.stat_list
    assert len(list(l)) == 0

    c = ctx.Channel("echo://;name=echo", stat='yes')

    assert len(list(l)) == 1

    assert [x.name for x in l] == ['echo']

    c.open()
    assert c.state == c.State.Opening
    c.process()
    assert c.state == c.State.Active

    #assert [x.swap() for x in l] == []
    assert [(f.name, f.value) for f in list(l)[0].swap()] == [('rx', 0), ('rx', 0), ('tx', 0), ('tx', 0)]

    c.post(b'xxx', seq=100)

    assert [(f.name, f.value) for f in list(l)[0].swap()] == [('rx', 1), ('rx', 3), ('tx', 1), ('tx', 3)]

def test_derived(context):
    context.register(Echo)
    v1 = context.Channel("echo://;name=echo")
    v1.open()
    assert v1.config['info.echo'] == 'yes'

    context.register(EchoV2)
    v2 = context.Channel("echo-v2://;name=echo-v2")
    v2.open()
    assert v2.config['info.echo'] == 'v2'

    v11 = context.Channel("echo://;name=echo-v1-1")
    v11.open()
    assert v11.config['info.echo'] == 'yes'
