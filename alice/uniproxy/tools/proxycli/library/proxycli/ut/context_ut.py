from alice.uniproxy.tools.proxycli.library.proxycli.context import Context


def test_context_construction():
    c = Context()
    assert str(c) == 'root'


def test_context_variable():
    c = Context()
    c.foo = 10
    assert c.foo == 10
    assert c.bar is None


def test_nested_context_set():
    a = Context('root')
    a.foo = 10

    b = Context('root.nested', parent=a)
    b.bar = 20

    c = Context('root.nested.nested', parent=b)
    c.baz = 30

    assert a.foo == 10
    assert b.foo == 10
    assert c.foo == 10

    assert a.bar is None
    assert b.bar == 20
    assert b.bar == 20

    assert a.baz is None
    assert b.baz is None
    assert c.baz == 30

    c.bar = 30
    assert a.bar is None
    assert b.bar == 20
    assert c.bar == 30

    b.foo = 20
    c.foo = 30
    assert a.foo == 10
    assert b.foo == 20
    assert c.foo == 30


def test_nested_delattr():
    a = Context()
    a.foo = 10

    b = Context('root.nested', parent=a)
    b.foo = 20

    assert b.foo == 20
    del b.foo

    assert b.foo == 10
    del b.foo

    assert b.foo == 10


def test_with_data():
    a = Context('root', tag='root', data={
        'foo': 10,
        'bar': '20'
    })

    assert a.foo == 10
    assert a.bar == '20'


def test_nested_with_data():
    a = Context('root', tag='root')
    b = a('nested', tag='nested', data={
        'foo': 10,
        'bar': '20'
    })

    assert b.foo == 10
    assert b.bar == '20'


def test_find_tagged_parent():
    a = Context(tag='root')
    a.foo = 10
    b = a('root.session', tag='session')
    b.foo = 20

    c = b('root.session.asr')
    c.foo = 30

    assert c['session'] is not None
    assert c['session'].foo is not None
    assert c['session'].foo == 20

    assert c['root'].foo is not None
    assert c['root'].foo == 10
