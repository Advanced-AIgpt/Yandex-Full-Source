# coding: utf-8

from __future__ import unicode_literals

from vins_core.common.utterance import Utterance
from vins_core.nlu.features.base import SampleFeatures
from vins_core.utils.data import open_resource_file


def test_unicode_payload():
    utt = Utterance(
        text='русский текст',
        input_source=Utterance.TEXT_INPUT_SOURCE,
        payload={'русский ключ': 'русское значение'}
    )
    utt_bytes = utt.to_bytes()
    utt2 = Utterance.from_bytes(utt_bytes)
    assert utt.text == utt2.text
    assert utt.input_source == utt2.input_source
    assert utt.payload == utt2.payload


def test_deserialization():
    path = 'vins_core/test/test_data/protobuf/sample_features'
    sf_proto_bin = open_resource_file(path, encoding=None).read()
    sample_features = SampleFeatures.from_bytes(sf_proto_bin)
    assert isinstance(sample_features, SampleFeatures)
    assert sample_features.sample.utterance.text == 'какая погода'
