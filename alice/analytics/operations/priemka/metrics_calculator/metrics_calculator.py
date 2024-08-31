# coding: utf-8

import math
from inspect import isfunction
from alice.analytics.operations.priemka.metrics_calculator.metrics import calc_diff_percent


class MetricCalculator(object):
    def __init__(self, metric, data, query_keys, metric_params=None):
        self.metric = metric
        self.metric.set_params(metric_params)
        self.metric_name = metric.name()
        self.metrics_group = metric.group()
        self.compare_nulls_policy = metric.compare_nulls_policy()
        self.negative_queries_policy = metric.negative_queries_policy()
        self.calculation_nulls_policy = metric.calculation_nulls_policy()
        self.metric_statistical_test = metric.statistical_test()
        self.metric_aggregate_type = self.metric.aggregate_type()
        self.prior = metric.prior()
        self.query_keys = self._filter_keys(data, query_keys)

        self.prod_data = data
        self.values_dict = {key: metric.value(data.get(key, {})) for key in self.query_keys}

    def is_defined(self):
        return len([1 for x in self.values_dict.values() if x is not None]) > 0

    def _filter_keys(self, data, keys):
        negative_policies = {
            'default': {0, None},
            'defined': {0, 1},
            'any': {0, 1, None},
            'only_negatives': {1},
        }

        filtered_keys = []
        for key in keys:
            value = data.get(key, {})
            if value and value.get('is_negative_query') in negative_policies[self.negative_queries_policy]:
                filtered_keys.append(key)

        return sorted(filtered_keys)

    def _get_values_list(self):
        values_list = []
        for key in self.query_keys:
            if self.calculation_nulls_policy == 'all':
                values_list.append(_get(self.values_dict, key, self.prior))
            elif self.calculation_nulls_policy == 'notnulls' and self.values_dict.get(key) is not None:
                values_list.append(self.values_dict.get(key))
        return values_list

    def _calc_value(self):
        values_list = self._get_values_list()

        metric_num, metric_denom, metric_value = None, None, None
        if self.metric_aggregate_type == 'ratio':
            metric_num = sum(values_list)
            metric_denom = float(len(values_list) or 1.)
            metric_value = metric_num / metric_denom
        elif self.metric_aggregate_type == 'sum':
            metric_num = sum(values_list)
            metric_denom = 1.0
            metric_value = metric_num / metric_denom
        elif self.metric_aggregate_type == 'custom':
            return self.metric.compare(values_list, None)
        elif self.metric_aggregate_type is None:
            # в этом режиме ничего не рассчитываем
            pass

        return metric_num, metric_denom, metric_value

    def compare_metric(self, prod_data, test_data):
        return self.metric.compare(prod_data, test_data)

    def calc(self):
        if not self.is_defined():
            return None

        metric_num, metric_denom, metric_value = self._calc_value()

        result = dict(
            metric_name=self.metric_name,
            metrics_group=self.metrics_group,
            metric_num=metric_num,
            metric_denom=metric_denom,
            metric_value=metric_value,
        )

        if self.metric.__doc__:
            result['metric_description'] = self.metric.__doc__

        return result


class MetricComparator(object):
    def __init__(self, prod_calculator, test_calculator):
        self.prod_calculator = prod_calculator
        self.test_calculator = test_calculator
        self.STAT_TESTS = {
            'ttest_rel': calc_pvalue_ttest_relative,
            # 'wilcoxon': compute_wx_test,
        }

    def _get_values_lists(self):
        nulls_policy = self.prod_calculator.compare_nulls_policy
        prod_values_dict, test_values_dict = self.prod_calculator.values_dict, self.test_calculator.values_dict
        prod_values_list, test_values_list = [], []

        if isfunction(nulls_policy):
            # кастомный способ расчёта значений в проде и тесте
            return nulls_policy(self.prod_calculator.query_keys, prod_values_dict, test_values_dict)

        for key in self.prod_calculator.query_keys:
            if nulls_policy == 'all':
                prod_values_list.append(_get(prod_values_dict, key, self.prod_calculator.prior))
                test_values_list.append(_get(test_values_dict, key, self.test_calculator.prior))
            if nulls_policy == 'union' and (prod_values_dict.get(key) is not None or test_values_dict.get(key) is not None):
                prod_values_list.append(_get(prod_values_dict, key, self.prod_calculator.prior))
                test_values_list.append(_get(test_values_dict, key, self.test_calculator.prior))
            elif nulls_policy == 'intersection' and (prod_values_dict.get(key) is not None and test_values_dict.get(key) is not None):
                prod_values_list.append(prod_values_dict.get(key))
                test_values_list.append(test_values_dict.get(key))
        return prod_values_list, test_values_list

    def _calc_significance(self):
        stat_test_function = self.STAT_TESTS.get(self.prod_calculator.metric_statistical_test)
        if stat_test_function is None:
            return None

        prod_values_list, test_values_list = self._get_values_lists()

        return stat_test_function(prod_values_list, test_values_list)

    def calc(self):
        if not self.prod_calculator.is_defined() and not self.test_calculator.is_defined():
            # выходим если ни в одной из систем не посчиталась метрика
            return None

        prod_quality, test_quality, diff, diff_percent = None, None, None, None
        if self.prod_calculator.metric_aggregate_type == 'custom':
            prod_values_list, test_values_list = self._get_values_lists()
            prod_quality, test_quality, diff, diff_percent = self.prod_calculator.compare_metric(prod_values_list, test_values_list)
        elif self.prod_calculator.metric_aggregate_type is None:
            # в этом режиме ничего не рассчитываем
            pass
        else:
            prod_result = self.prod_calculator.calc()
            prod_quality = prod_result['metric_value'] if prod_result else None

            test_result = self.test_calculator.calc()
            test_quality = test_result['metric_value'] if test_result else None

            if prod_quality and test_quality:
                diff = test_quality - prod_quality
                diff_percent = calc_diff_percent(prod_quality, test_quality)

        pvalue = None
        if self.prod_calculator.metric_statistical_test is not None:
            pvalue = self._calc_significance()

        result = dict(
            metric_name=self.prod_calculator.metric_name,
            metrics_group=self.prod_calculator.metrics_group,
            prod_quality=prod_quality,
            test_quality=test_quality,
            diff=diff,
            diff_percent=diff_percent,
            pvalue=pvalue,
        )

        if self.prod_calculator.metric.__doc__:
            result['metric_description'] = self.prod_calculator.metric.__doc__

        return result


def _get(container, key, prior=None):
    value = container.get(key)
    return prior if value is None else value


def calc_pvalue_ttest_relative(prod_values, test_values):
    from scipy.stats import ttest_rel

    ttest = ttest_rel(prod_values, test_values)
    pvalue = ttest[1] if isinstance(ttest, tuple) else ttest.pvalue
    if math.isnan(pvalue):
        if prod_values == test_values:
            return 1.0
        else:
            return None
    return pvalue
