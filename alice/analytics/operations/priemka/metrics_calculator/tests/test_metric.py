from alice.analytics.operations.priemka.metrics_calculator.metric import Metric, BaseMetric
from alice.analytics.operations.priemka.metrics_calculator.calc_metrics_local import need_calc_this_metric


def test_metric_1():
    metric = Metric()
    assert metric.name() == 'metric'
    assert metric.group() is None


def test_metric_2():
    class TestMetric(Metric):
        def group(self):
            return 'qqq'

    metric = TestMetric()
    assert metric.name() == 'test_metric'
    assert metric.group() == 'qqq'
    assert metric.aggregate_type() == 'ratio'
    assert metric.calculation_nulls_policy() == 'notnulls'
    assert metric.compare_nulls_policy() == 'intersection'
    assert metric.statistical_test() == 'ttest_rel'


def test_need_calc_this_metric_01():
    class TestMetric(Metric):
        def group(self):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is False


def test_need_calc_this_metric_02():
    class TestMetric(Metric):
        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is True


def test_need_calc_this_metric_03():
    class TestMetric(Metric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is False


def test_need_calc_this_metric_04():
    class TestMetric(Metric):
        calc_this_metric = True

        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is True


def test_need_calc_this_metric_05():
    class ParentMetric(Metric):
        pass

    class TestMetric(ParentMetric):
        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is True
    assert need_calc_this_metric(ParentMetric()) is False


def test_need_calc_this_metric_06():
    class ParentMetric(Metric):
        pass

    class TestMetric(ParentMetric):
        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is True
    assert need_calc_this_metric(ParentMetric()) is False


def test_need_calc_this_metric_07():
    class ParentMetric(Metric):
        def value(self, item):
            return 'qqq'

    class TestMetric(ParentMetric):
        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is True
    assert need_calc_this_metric(ParentMetric()) is True


def test_need_calc_this_metric_08():
    class ParentMetric(Metric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    class TestMetric(ParentMetric):
        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is True
    assert need_calc_this_metric(ParentMetric()) is False


def test_need_calc_this_metric_09():
    class ParentMetric(Metric):
        calc_this_metric = False

    class TestMetric(ParentMetric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is False
    assert need_calc_this_metric(ParentMetric()) is False


def test_need_calc_this_metric_10():
    class ParentMetric(Metric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    class TestMetric(BaseMetric, ParentMetric):
        pass

    assert need_calc_this_metric(TestMetric()) is True
    assert need_calc_this_metric(ParentMetric()) is False


def test_need_calc_this_metric_11():
    class ParentMetric(Metric):
        pass

    class TestMetric(BaseMetric, ParentMetric):
        pass

    assert need_calc_this_metric(TestMetric()) is False
    assert need_calc_this_metric(ParentMetric()) is False


def test_need_calc_this_metric_12():
    class ParentMetric(Metric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    class TestMetric(BaseMetric, ParentMetric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is False
    assert need_calc_this_metric(ParentMetric()) is False


def test_need_calc_this_metric_13():
    class ParentMetric(Metric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    class TestMetric(ParentMetric):
        calc_this_metric = False

        def value(self, item):
            return 'qqq'

    assert need_calc_this_metric(TestMetric()) is False
    assert need_calc_this_metric(ParentMetric()) is False
