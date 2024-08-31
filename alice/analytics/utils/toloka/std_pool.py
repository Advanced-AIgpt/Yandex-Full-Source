#!/usr/bin/env python
# encoding: utf-8
from __future__ import unicode_literals

import logging

from utils.nirvana.op_caller import get_current_workflow_url

from api import PoolRequester
from filters import RU_LANG_FILTER


class StdPool(PoolRequester):
    def __init__(self, prj_id, skill_id, training_pool_id=None, training_passing_skill_value=None, pool_id=None,
                 label_duration=None, label_count=None, label_avg=None, page_size=None,
                 # Должен быть задан либо label_avg, либо page_size, либо два других
                 out_spec=None, random_accuracy=None,  # Должно быть задано хотя бы что-то из них
                 overlap=5,  # TODO: Вычислять на основе заказанного уровня точности
                 priority=50,
                 price_intervals=None,
                 golden_task_distribution_function_intervals=(
                    (1, 2),  # Первый лист - экзаменационный
                    (2, 3),  # 2й и 3й лист - без полного доверия
                    (4, 5),  # В штатном режиме
                    (20, 8)  # Доверенный исполнитель
                 ),
                 increase_overlap_for_banned=False,
                 task_limit_for_user=None,
                 **kwargs):
        super(StdPool, self).__init__(prj_id=prj_id, pool_id=pool_id, **kwargs)
        self.prj_id = prj_id
        self.pool_id = pool_id
        self.skill_id = skill_id
        self.training_pool_id = training_pool_id
        self.training_passing_skill_value = training_passing_skill_value
        self.price_intervals = price_intervals
        self.golden_task_distribution_function_intervals = golden_task_distribution_function_intervals

        if page_size:
            self.page_size = page_size
        else:
            if label_avg is not None:
                label_duration = label_avg
                label_count = 1
            self.page_size = self._calc_page_size(label_duration, label_count)

        self.random_accuracy = random_accuracy or self._calc_random_accuracy(out_spec)
        self.acc_q = AccuracyQualifier(self.random_accuracy)

        self.overlap = overlap
        self.priority = priority
        self.increase_overlap_for_banned = increase_overlap_for_banned
        self.task_limit_for_user = task_limit_for_user

    def _make_pool_props(self):
        url = get_current_workflow_url()
        props = {
            "project_id": self.prj_id,
            "private_name": "Auto generated StdPool",
            "private_comment": "Created from local script" if url is None else 'Created from %s' % url,
            "public_description": "",
            "reward_per_assignment": 0.02,
            # may_contain_adult_content (имеет ли смысл устанавливать в False? Гарантий то нет)
            # Стараемся рассчитывать листы по 2 минуты, но мало ли над чем человек задумается...
            "assignment_max_duration_seconds": 1800,
            "auto_accept_solutions": True,
            "auto_close_after_complete_delay_seconds": 10,
            "priority": self.priority,
            "will_expire": self.ttl_to_expire(),
        }
        self._add_worker_filters(props)
        self._add_mixer_props(props)
        self._add_quality_control(props)
        self._add_dynamic_pricing(props)
        return props

    def _add_mixer_props(self, props):
        props["defaults"] = {
            "default_overlap_for_new_tasks": self.overlap,
            "default_overlap_for_new_task_suites": self.overlap,
        }
        props["mixer_config"] = {
            "real_tasks_count": self.page_size,
            "golden_tasks_count": 0,
            "training_tasks_count": 0,
            #"min_real_tasks_count": None,
            "min_golden_tasks_count": None,
            "min_training_tasks_count": 0,
            "force_last_assignment": True,
            "mix_tasks_in_creation_order": False,
            "shuffle_tasks_in_task_suite": True,
        }

        if self.golden_task_distribution_function_intervals is not None:
            props["mixer_config"]["golden_task_distribution_function"] = {
                "scope": "PROJECT",
                "distribution": "UNIFORM",
                "window_days": 7,
                "intervals": list(self._gen_gs_rate(self.golden_task_distribution_function_intervals)),
            }
            #"training_task_distribution_function": # TODO (?) Уметь подсоединять обучение на первые страницы
        return props

    def _gen_gs_rate(self, intervals):
        # intervals - пары из номеров страниц и частоты показа ханипотов на них
        end = [idx for idx, freq in intervals[1:]]
        end.append(None)
        for (first, freq), last in zip(intervals, end):
            yield {'from': (first - 1) * self.page_size + 1,
                   'to': None if last is None else (last - 1) * self.page_size,
                   'frequency': freq}

    def _add_worker_filters(self, props):
        # Даже если условие одно, нужно завернуть его в "and" и "or"
        props["filter"] = {"and": [RU_LANG_FILTER]}
        return props

    # Контроль качества

    def _add_quality_control(self, props):
        configs = [
            self._too_many_skipped_rule(),
            self._set_skill_rule(),
            self.acc_q.ban_rule(int(round(self.page_size * 0.4)), 0.9),
            self.acc_q.ban_rule(self.page_size, 1.2),
        ]

        if self.increase_overlap_for_banned:
            configs.append(self._increase_overlap_for_banned_rule())

        if self.task_limit_for_user is not None:
            configs.append(self._too_many_tasks_for_user_rule())

        props['quality_control'] = {
            'configs': configs,
        }

        if self.training_pool_id:
            props['quality_control']['training_requirement'] = self._create_training_requirement()
        return props

    def _too_many_skipped_rule(self):
        return {
            'collector_config': {'parameters': {}, 'type': 'SKIPPED_IN_ROW_ASSIGNMENTS'},
            'rules': [{
                'action': {
                    'parameters': {
                        'duration_days': 1,
                        'private_comment': 'Автобан по пропущенным заданиям',
                        'scope': 'POOL'},
                    'type': 'RESTRICTION'},
                'conditions': [{'key': 'skipped_in_row_count', 'operator': 'GT', 'value': 4}]
            }]}

    def _increase_overlap_for_banned_rule(self):
        return {
            'collector_config': {'type': 'USERS_ASSESSMENT'},
            'rules': [{
                'conditions': [
                    {
                        'key': 'pool_access_revoked_reason',
                        'operator': 'EQ',
                        'value': 'RESTRICTION'
                    }
                ],
                'action': {
                    'type': 'CHANGE_OVERLAP',
                    'parameters': {
                        'delta': 1,
                        'open_pool': True
                    }
                }
            }]}

    def _too_many_tasks_for_user_rule(self):
        return {
            'collector_config': {'type': 'ANSWER_COUNT'},
            'rules': [{
                'conditions': [
                    {
                        'key': 'assignments_accepted_count',
                        'operator': 'GTE',
                        'value': self.task_limit_for_user
                    }
                ],
                'action': {
                    'type': 'RESTRICTION',
                    'parameters': {
                        'scope': 'POOL',
                        'duration_days': 1
                    }
                }
            }]}

    def _set_skill_rule(self):
        return {
            'collector_config': {'parameters': {'history_size': self.page_size},
                                 'type': 'GOLDEN_SET'},
            'rules': [{
                'action': {
                    'parameters': {
                        'from_field': 'golden_set_correct_answers_rate',
                        'skill_id': self.skill_id
                    },
                    'type': 'SET_SKILL_FROM_OUTPUT_FIELD'
                },
                'conditions': [{'key': 'golden_set_answers_count',
                                'operator': 'GTE',
                                'value': int(self.page_size * 0.5)}]
            }]}

    def _create_training_requirement(self):
        return {
            "training_passing_skill_value": self.training_passing_skill_value,
            "training_pool_id": self.training_pool_id
        }

    # Динамическое ценообразование

    def _add_dynamic_pricing(self, props):
        intervals = list(self.acc_q.iter_price_intervals(self.price_intervals))
        props['dynamic_pricing_config'] = {'intervals': intervals,
                                           'skill_id': self.skill_id,
                                           'type': 'SKILL'}
        return props

    # Подсчёты

    def _calc_page_size(self, duration, count=50):
        # Вычисление количества заданий на странице, ориентируясь на секундомер создающего
        return int(round(2.0 * count / duration))  # Хотим, чтобы страница выполнялась ~2 минуты

    def _calc_random_accuracy(self, out_spec):
        # Какой скилл покажет толокер, если будет тыкать в селекторы случайным образом
        acc = 1.0
        for field in out_spec:
            if 'rand_accuracy' in field:
                # Экспертная оценка от создающего
                acc *= field['rand_accuracy']
            elif field['type'] == 'boolean':
                acc *= 0.5
            elif field['type'] == 'selector':
                acc /= len(field['choices'])
            # Далее идут опции с потенциально бесконечным числом вариантов,
            # лучше для них указывать 'rand_accuracy'
            elif field['type'] == 'integer':
                logging.warning('Better to set explicit rand_accuracy for %s' % field)
                acc *= 0.3
            elif field['type'] in ('string', 'float'):
                logging.warning('Better to set explicit rand_accuracy for %s' % field)
                acc *= 0.15
            else:
                UserWarning('Unknown field type: %s' % field)

        return acc


class AccuracyQualifier(object):
    def __init__(self, random_accuracy):
        self.random_accuracy = random_accuracy

    def iter_price_intervals(self, intervals=None):
        """

        :param None|list|dict intervals: Интервалы с оплатой. Представляют из себя пары чисел.
            Первое число - граница интервала (значение навыка)
            Второе число - оплата в долларах
        :return:
        """
        if intervals is None:
            intervals = ((int(self.acc_level(degree)), price)
                         for degree, price in [(3, 0.03), (4, 0.04), (5, 0.05), (8, 0.06)])
            prev_price = 0.02
        else:
            if isinstance(intervals, dict):
                intervals = sorted(intervals.iteritems())

            first_bound, first_price = intervals[0]

            if first_bound == 0:
                prev_price = first_price
                intervals = intervals[1:]
            elif first_price == 0.01:
                prev_price = 0.01
                intervals = intervals[1:]
            else:
                prev_price = first_price - 0.01

        prev_acc = 0
        for acc, price in intervals:
            yield {'from': prev_acc, 'reward_per_assignment': prev_price, 'to': acc-1}
            prev_acc = acc
            prev_price = price
        yield {'from': prev_acc, 'reward_per_assignment': prev_price, 'to': 100}

    def acc_level(self, degree):
        # degree = 1 уровень рандомного тыка
        return round(100 * (1 - (1 - self.random_accuracy) ** degree), 1)

    def ban_rule(self, history_size, degree):
        acc = self.acc_level(degree)
        return {
            'collector_config': {'parameters': {'history_size': history_size},
                                 'type': 'GOLDEN_SET'},
            'rules': [{
                'action': {
                    'parameters': {
                        'duration_days': 5,
                        'private_comment': 'Слишком много ошибок на %s ответах' % history_size,
                        'scope': 'PROJECT'},
                    'type': 'RESTRICTION'},
                'conditions': [{'key': 'golden_set_answers_count',
                                'operator': 'GTE',
                                'value': history_size},
                               {'key': 'golden_set_correct_answers_rate',
                                'operator': 'LT',
                                'value': acc}]}]
        }
