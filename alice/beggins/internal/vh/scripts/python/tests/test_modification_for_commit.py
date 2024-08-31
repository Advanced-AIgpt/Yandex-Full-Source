import tempfile

from alice.beggins.internal.vh.scripts.python.modification_for_commit import (
    DataIncModifier, FramesPbTxtModifier,
)


def test_data_inc_modifier():
    file = tempfile.TemporaryFile(mode='r+')
    file.write('''
FROM_SANDBOX(
    3235709471
    RENAME
    guru/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaGuru/model.cbm
)

SET(
    ${PREFIX}zeliboba_models/AliceZelibobaGuru/model.cbm
)
'''.lstrip())

    data_inc_modifier = DataIncModifier(file)
    data_inc_modifier.modify({
        'model_name': 'test_model',
        'resource_id': 228,
        'embedder': 'Zeliboba',
        'process_id': '1234',
    })

    expected = '''
FROM_SANDBOX(
    3235709471
    RENAME
    guru/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaGuru/model.cbm
)

FROM_SANDBOX(
    228
    RENAME
    test_model/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaTestModel__1234/model.cbm
)

SET(
    ${PREFIX}zeliboba_models/AliceZelibobaGuru/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaTestModel__1234/model.cbm
)
'''.lstrip()

    tested = file.read()

    assert expected == tested


def test_frames_pb_txt_modifier():
    file = tempfile.TemporaryFile(mode='r+')
    file.write('''
Language: "ru"
Frames: [
  {
    Name: "alice.market.how_much"
    Experiments: ["bg_market_how_much_on_binary_classifier"]
    Rules: [
      {
        Classifier: {
          Source: "Granet"
          Intent: "alice.market.how_much.negative_fixlist"
          IsNegative: true
        }
      }
    ]
  }
]
'''.lstrip())

    frames_pb_txt_modifier = FramesPbTxtModifier(file)
    frames_pb_txt_modifier.modify({
        'model_name': 'test',
        'threshold': 1337,
        'embedder': 'Zeliboba',
        'process_id': '1234',
    })

    expected = '''
Language: "ru"
Frames: [
  {
    Name: "alice.market.how_much"
    Experiments: ["bg_market_how_much_on_binary_classifier"]
    Rules: [
      {
        Classifier: {
          Source: "Granet"
          Intent: "alice.market.how_much.negative_fixlist"
          IsNegative: true
        }
      }
    ]
  },
  {
    Name: "alice.beggins_test__1234"
    Rules: [{
      Experiments: ["bg_beggins_AliceZelibobaTest__1234"]
      Classifier: {
        Source: "AliceBinaryIntentClassifier"
        Intent: "AliceZelibobaTest__1234"
        Threshold: 1337
        Confidence: 1
      }
      Tagger: {
        Source: "Always"
      }
    }]
  }
]
'''.lstrip()

    tested = file.read()

    assert expected == tested
