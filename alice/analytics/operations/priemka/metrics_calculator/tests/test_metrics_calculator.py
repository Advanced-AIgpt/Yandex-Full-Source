from pytest import approx
from alice.analytics.operations.priemka.metrics_calculator import metrics_calculator
from alice.analytics.operations.priemka.metrics_calculator.metric import Metric
from alice.analytics.operations.priemka.metrics_calculator.calc_metrics_local import prepare_items_dict


def test__get():
    assert 5 == metrics_calculator._get({'key': 5}, 'key')
    assert 42 == metrics_calculator._get({'key': 5}, 'key2', 42)
    assert metrics_calculator._get({'key': 5}, 'key2') is None


def test_calc_diff_percent():
    assert 100.0 == metrics_calculator.calc_diff_percent(0, 1)
    assert 0.0 == metrics_calculator.calc_diff_percent(0, 0)
    assert 0.0 == metrics_calculator.calc_diff_percent(0.1, 0.1)
    assert 100.0 == metrics_calculator.calc_diff_percent(0.001, 0.002)
    assert -25.0 == metrics_calculator.calc_diff_percent(8, 6)
    assert -100.0 == metrics_calculator.calc_diff_percent(1, 0)
    assert -200.0 == metrics_calculator.calc_diff_percent(1, -1)


def test_calc_diff_percent_sign():
    """
    ------------------------------------------------------------------->
                -4  -3  -2  -1  0   1   2   3   4
                                        a       b
                                        b       a
                a      b
                b      a
                                a   b
                                b   a
                            a   b
                            b   a
                            a       b
                            b       a
                b                   a
                a                   b
    """
    assert 100.0 == metrics_calculator.calc_diff_percent(2, 4)
    assert -50.0 == metrics_calculator.calc_diff_percent(4, 2)
    assert 50.0 == metrics_calculator.calc_diff_percent(-4, -2)
    assert -100.0 == metrics_calculator.calc_diff_percent(-2, -4)
    assert 100.0 == metrics_calculator.calc_diff_percent(0, 1)
    assert -100.0 == metrics_calculator.calc_diff_percent(1, 0)
    assert 100.0 == metrics_calculator.calc_diff_percent(-1, 0)
    assert -100.0 == metrics_calculator.calc_diff_percent(0, -1)
    assert 200.0 == metrics_calculator.calc_diff_percent(-1, 1)
    assert -200.0 == metrics_calculator.calc_diff_percent(1, -1)
    assert -500.0 == metrics_calculator.calc_diff_percent(1, -4)
    assert 125.0 == metrics_calculator.calc_diff_percent(-4, 1)


def test_calc_pvalue_ttest_relative():
    assert 0.0 == metrics_calculator.calc_pvalue_ttest_relative([1] * 1000, [0] * 1000)
    assert 0.0 == metrics_calculator.calc_pvalue_ttest_relative([1] * 100, [0] * 100)
    assert 0.0 == metrics_calculator.calc_pvalue_ttest_relative([1] * 10, [0] * 10)
    assert 1.0 == metrics_calculator.calc_pvalue_ttest_relative([1, 0], [0, 1])

    assert 0.0 == metrics_calculator.calc_pvalue_ttest_relative([0.5, 0.5, 0.5, 0.5], [0.6, 0.6, 0.6, 0.6])
    assert approx(0.39, 0.01) == metrics_calculator.calc_pvalue_ttest_relative([0.5, 0.5, 0.5, 0.5], [0.6, 0.6, 0.6, 0.4])
    assert 1.0 == metrics_calculator.calc_pvalue_ttest_relative([0.5, 0.5, 0.5, 0.5], [0.6, 0.6, 0.4, 0.4])
    assert approx(0.39, 0.01) == metrics_calculator.calc_pvalue_ttest_relative([0.5, 0.5, 0.5, 0.5], [0.6, 0.4, 0.4, 0.4])
    assert 0.0 == metrics_calculator.calc_pvalue_ttest_relative([0.5, 0.5, 0.5, 0.5], [0.4, 0.4, 0.4, 0.4])
    assert approx(0.373, 0.01) == metrics_calculator.calc_pvalue_ttest_relative([0.5, 0.5, 0.5, 0.5, 0.5], [0.6, 0.4, 0.4, 0.4, 0.5])
    assert approx(0.01613, 0.01) == metrics_calculator.calc_pvalue_ttest_relative([0, 0, 0, 0, 0], [1, 1, 1, 1, 0])


class TestMetricCalculator():
    def setup_class(self):
        class MyMetric(Metric):
            def value(self, item):
                return item.get('metric')
        self.metric = MyMetric()

    def init_calculator(self, data_list):
        data = [{'req_id': str(i), 'metric': x} for i, x in enumerate(data_list)]
        keys = set([x['req_id'] for x in data])
        return metrics_calculator.MetricCalculator(self.metric, prepare_items_dict(data), keys)

    def test_base_init(self):
        assert self.init_calculator([1]) is not None

    def test_is_defined(self):
        assert self.init_calculator([1, 1, 2, 2]).is_defined() is True
        assert self.init_calculator([1]).is_defined() is True
        assert self.init_calculator([]).is_defined() is False
        assert self.init_calculator([None, None]).is_defined() is False

    def test_calc_value(self):
        # metric_aggregate_type == 'ratio'
        assert self.init_calculator([1, 1, 2, 2])._calc_value() == (6, 4, 1.5)
        assert self.init_calculator([1])._calc_value() == (1, 1, 1)
        assert self.init_calculator([1, 1])._calc_value() == (2, 2, 1)
        assert self.init_calculator([1, 10])._calc_value() == (11, 2, 5.5)
        assert self.init_calculator([0.5, -0.5])._calc_value() == (0, 2, 0)

        # metric_aggregate_type == 'sum'
        calculator = self.init_calculator([1, 1])
        calculator.metric_aggregate_type = 'sum'
        assert calculator._calc_value() == (2, 1, 2)

        calculator = self.init_calculator([1, None, 2.5])
        calculator.metric_aggregate_type = 'sum'
        assert calculator._calc_value() == (3.5, 1.0, 3.5)

    def test_get_values_list(self):
        # calculation_nulls_policy == 'notnulls'
        assert self.init_calculator([1, 1, 2, 2])._get_values_list() == [1, 1, 2, 2]
        assert self.init_calculator([1, None, 2])._get_values_list() == [1, 2]

        # calculation_nulls_policy == 'all'
        calculator = self.init_calculator([1, 1, 2, 2])
        calculator.calculation_nulls_policy = 'all'
        assert calculator._get_values_list() == [1, 1, 2, 2]

        calculator = self.init_calculator([1, None, 2])
        calculator.calculation_nulls_policy = 'all'
        assert calculator._get_values_list() == [1, None, 2]


METRIC_PRIOR = 42


class TestMetricComparator():
    def setup_class(self):
        class MyMetric(Metric):
            def value(self, item):
                return item.get('metric')

            def prior(self):
                return METRIC_PRIOR

            def compare(self, data1, data2):
                return sum(data1) * 2, sum(data2) * 2, (sum(data2) - sum(data1)) * 2, 777
        self.metric = MyMetric()

    def init_calculator(self, data_list):
        data = [{'req_id': str(i), 'metric': x} for i, x in enumerate(data_list)]
        keys = set([x['req_id'] for x in data])
        return metrics_calculator.MetricCalculator(self.metric, prepare_items_dict(data), keys)

    def init_comparator(self, data1, data2):
        calc1 = self.init_calculator(data1)
        calc2 = self.init_calculator(data2)
        return metrics_calculator.MetricComparator(calc1, calc2)

    def test_init(self):
        assert self.init_comparator([0, 0, 0, 0], [0, 0, 0, 1]) is not None

    def test_get_values_lists(self):
        cmp = self.init_comparator([0, 0, 0, 0], [0, 0, 0, 1])
        values1, values2 = cmp._get_values_lists()
        assert values1 == [0, 0, 0, 0] and values2 == [0, 0, 0, 1]

        cmp = self.init_comparator([1, 2, None, 4], [1, 2, 3, None])
        cmp.prod_calculator.compare_nulls_policy = 'union'
        values1, values2 = cmp._get_values_lists()
        assert values1 == [1, 2, METRIC_PRIOR, 4] and values2 == [1, 2, 3, METRIC_PRIOR]

        cmp = self.init_comparator([None, 2, None, 4], [None, 2, 3, None])
        cmp.prod_calculator.compare_nulls_policy = 'union'
        values1, values2 = cmp._get_values_lists()
        assert values1 == [2, METRIC_PRIOR, 4] and values2 == [2, 3, METRIC_PRIOR]

        # compare_nulls_policy = 'intersection'
        cmp = self.init_comparator([1, 2, None, 4], [1, 2, 3, None])
        cmp.prod_calculator.compare_nulls_policy = 'intersection'
        values1, values2 = cmp._get_values_lists()
        assert values1 == [1, 2] and values2 == [1, 2]

        cmp = self.init_comparator([None, 2, None, 4], [None, 2, 3, None])
        cmp.prod_calculator.compare_nulls_policy = 'intersection'
        values1, values2 = cmp._get_values_lists()
        assert values1 == [2] and values2 == [2]

        # compare_nulls_policy = 'all'
        cmp = self.init_comparator([1, 2, None, 4], [1, 2, 3, None])
        cmp.prod_calculator.compare_nulls_policy = 'all'
        values1, values2 = cmp._get_values_lists()
        assert values1 == [1, 2, METRIC_PRIOR, 4] and values2 == [1, 2, 3, METRIC_PRIOR]

        cmp = self.init_comparator([None, 2, None, 4], [None, 2, 3, None])
        cmp.prod_calculator.compare_nulls_policy = 'all'
        values1, values2 = cmp._get_values_lists()
        assert values1 == [METRIC_PRIOR, 2, METRIC_PRIOR, 4] and values2 == [METRIC_PRIOR, 2, 3, METRIC_PRIOR]

    def test_calc(self):
        cmp = self.init_comparator([0, 0, 0, 1], [0, 0, 0, 4])
        result = cmp.calc()
        assert result['metric_name'] == 'my_metric'
        assert result['prod_quality'] == 0.25
        assert result['test_quality'] == 1
        assert result['diff'] == 0.75
        assert result['diff_percent'] == 300.0
        assert result['pvalue'] == approx(0.391, 0.01)

        cmp = self.init_comparator([0, 0, 0, 1], [0, 0, 0, 4])
        cmp.prod_calculator.metric_aggregate_type = 'sum'
        cmp.test_calculator.metric_aggregate_type = 'sum'
        result = cmp.calc()
        assert result['prod_quality'] == 1
        assert result['test_quality'] == 4
        assert result['diff'] == 3
        assert result['diff_percent'] == 300.0
        assert result['pvalue'] == approx(0.391, 0.01)

        cmp = self.init_comparator([0, 0, 0, 1], [0, 0, 0, 4])
        cmp.prod_calculator.metric_aggregate_type = 'custom'
        result = cmp.calc()
        assert result['prod_quality'] == 2
        assert result['test_quality'] == 8
        assert result['diff'] == 6
        assert result['diff_percent'] == 777
        assert result['pvalue'] == approx(0.391, 0.01)
