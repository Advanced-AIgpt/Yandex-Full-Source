from alice.analytics.operations.priemka.metrics_calculator.metrics import CallsQualitySearchApp
from alice.analytics.operations.priemka.metrics_calculator.metrics import MessengerCallsScenario
from alice.analytics.operations.priemka.metrics_calculator.metrics import OnlyTargetAnswer
from alice.analytics.operations.priemka.metrics_calculator.metrics import WithTargetAnswer
from alice.analytics.operations.priemka.metrics_calculator.metrics import FirstTargetAnswer


def test_calls_quality_01():
    test_data = {'answer': [{'relevance': 'target'}]}
    assert CallsQualitySearchApp().value(test_data) == 1.


def test_calls_quality_02():
    test_data = {'answer': [{'relevance': 'target'}, {'relevance': 'stupid'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.84


def test_calls_quality_03():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'stupid'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.


def test_calls_quality_04():
    test_data = {'answer': [{'relevance': 'target'}, {'relevance': 'rel'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.95


def test_calls_quality_05():
    test_data = {'answer': [{'relevance': 'target'}, {'relevance': 'rel_minus'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.93


def test_calls_quality_06():
    test_data = {'answer': [{'relevance': 'rel'}, {'relevance': 'target'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.86


def test_calls_quality_07():
    test_data = {'answer': [{'relevance': 'rel_minus'}, {'relevance': 'target'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.77


def test_calls_quality_08():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'target'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.66


def test_calls_quality_09():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'target'}, {'relevance': 'stupid'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.54


def test_calls_quality_10():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'stupid'}, {'relevance': 'target'}]}
    assert CallsQualitySearchApp().value(test_data) == 0.45


def messenger_calls_scenario_01():
    test_data = {'intent': 'phone_call'}
    assert MessengerCallsScenario().value(test_data) == 1


def messenger_calls_scenario_02():
    test_data = {'intent': 'general_conversation'}
    assert MessengerCallsScenario().value(test_data) == 0


def only_target_answer_01():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'stupid'}, {'relevance': 'target'}]}
    assert OnlyTargetAnswer().value(test_data) == 0


def only_target_answer_02():
    test_data = {'answer': [{'relevance': 'target'}]}
    assert OnlyTargetAnswer().value(test_data) == 1


def with_target_answer_01():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'stupid'}, {'relevance': 'target'}]}
    assert WithTargetAnswer().value(test_data) == 1


def with_target_answer_02():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'stupid'}]}
    assert WithTargetAnswer().value(test_data) == 0


def first_target_answer_01():
    test_data = {'answer': [{'relevance': 'stupid'}, {'relevance': 'stupid'}, {'relevance': 'target'}]}
    assert FirstTargetAnswer().value(test_data) == 0


def first_target_answer_02():
    test_data = {'answer': [{'relevance': 'target'}, {'relevance': 'stupid'}, {'relevance': 'stupid'}]}
    assert FirstTargetAnswer().value(test_data) == 1
