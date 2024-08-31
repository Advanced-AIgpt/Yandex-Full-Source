from alice.paskills.penguinarium.ml.embedder import BaseEmbedder


def test_base_embedder():
    assert BaseEmbedder.embed(None, None) is None
