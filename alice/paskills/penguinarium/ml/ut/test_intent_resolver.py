from alice.paskills.penguinarium.ml.intent_resolver import BaseIntentResolver


def test_base_intent_resolver():
    assert BaseIntentResolver.build_model(None, None) is None
    assert BaseIntentResolver.resolve_intents(None, None, None) is None
