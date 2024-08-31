from personal_assistant.is_allowed_intent.config_loader import load_config
from personal_assistant.is_allowed_intent.resolver import Resolver


def test_resolver_1():
    from predicates.predicates1 import reg
    resolver = Resolver('a', range(0, 20), reg.predicates,
                        config=load_config('personal_assistant/tests/test_is_allowed_intent/configs/config1.yaml'))
    assert not resolver.process({'a': 5, 'b': 'b'})
    assert not resolver.process({'a': 5, 'b': 'a'})
    assert not resolver.process({'a': 5, 'b': 'c'})
    assert not resolver.process({'a': 5, 'b': 'd'})
    assert resolver.process({'a': 5, 'b': 'e'})
    assert resolver.process({'a': 5, 'b': 1})
    assert resolver.process({'a': 5, 'b': True})

    assert resolver.process({'a': 1, 'b': 'b'})
    assert resolver.process({'a': 2, 'b': 'a'})
    assert resolver.process({'a': 4, 'b': 'c'})
    assert resolver.process({'a': 5, 'b': 'e'})
    assert resolver.process({'a': 6, 'b': 1})
    assert resolver.process({'a': 7, 'b': True})

    assert not resolver.process({'a': 7, 'b': 'e'})
    assert not resolver.process({'a': 7, 'b': 'b'})
    assert resolver.process({'a': 1, 'b': 'e'})
    assert resolver.process({'a': 2, 'b': 'b'})

    assert not resolver.process({'a': 4, 'b': 'd'})


def test_resolver_2():
    from predicates.predicates1 import reg
    resolver = Resolver('a', [], reg.predicates,
                        config=load_config('personal_assistant/tests/test_is_allowed_intent/configs/config1.yaml'))
    assert not resolver.process({'a': 5, 'b': 'b'})
    assert not resolver.process({'a': 5, 'b': 'a'})
    assert not resolver.process({'a': 5, 'b': 'c'})
    assert not resolver.process({'a': 5, 'b': 'd'})
    assert resolver.process({'a': 5, 'b': 'e'})
    assert resolver.process({'a': 5, 'b': 1})
    assert resolver.process({'a': 5, 'b': True})

    assert resolver.process({'a': 1, 'b': 'b'})
    assert resolver.process({'a': 2, 'b': 'a'})
    assert resolver.process({'a': 4, 'b': 'c'})
    assert resolver.process({'a': 5, 'b': 'e'})
    assert resolver.process({'a': 6, 'b': 1})
    assert resolver.process({'a': 7, 'b': True})

    assert not resolver.process({'a': 7, 'b': 'e'})
    assert not resolver.process({'a': 7, 'b': 'b'})
    assert resolver.process({'a': 1, 'b': 'e'})
    assert resolver.process({'a': 2, 'b': 'b'})
