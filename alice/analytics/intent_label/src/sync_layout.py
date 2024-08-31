#!/usr/bin/env python
# encoding: utf-8
from string import Template
import json
from datetime import datetime

from utils.nirvana.op_caller import get_current_workflow_url
from utils.toloka.api import PoolRequester
from utils.toloka.filters import skill_filter, RU_LANG_FILTER, rkub_filter

from parse_conf import get_common_spec


class LayoutPool(PoolRequester):
    def __init__(self, layout_conf, prj_conf, pool_id=None, **kwargs):
        self.layout_conf = layout_conf  # LayoutConf
        self.prj_conf = prj_conf  # ProjectConf
        super(LayoutPool, self).__init__(prj_id=prj_conf.get_conf()['prj_id'],
                                         pool_id=pool_id,
                                         **kwargs)

    def list_training_suites(self):
        # Формат должен совпадать с выдачей из LayoutConf.list_training_suites()
        suites = []
        for s in self.list_task_suites():
            tasks = [{
                'dialog': t['input_values']['dialog'],
                'intent': t['known_solutions'][0]['output_values']['intent'] if 'known_solutions' in t else None,
                'hint': t.get('message_on_unknown_solution'),
            } for t in s['tasks']]
            suites.append(tasks)
        return suites

    def _make_pool_props(self):
        tmpl = Template(get_common_spec('exam_pool_props.json'))
        conf = self.layout_conf.get_conf()
        url = get_current_workflow_url()
        props = json.loads(tmpl.substitute(
            prj_id=self.prj_id,
            title=conf['title'],
            date=datetime.now().strftime('%Y-%m-%d'),
            # Хорошо бы ещё добавлять "%H:%M", но из-за временных поясов он может вводить в заблуждение
            skill_id=self.layout_conf.get_skill_id(self.prj_conf.get_owner()),
            private_comment="Created from local script\\nabc: ABC-23708" if url is None else 'Created from %s\\nabc: ABC-23708' % url,
            expire_date=self.ttl_to_expire(),
            tasks_count=self.layout_conf.count_hints(),
        ))
        self._add_worker_filters(props)
        return props

    def _add_worker_filters(self, props):
        filters = props['filter']['and']
        prj = self.prj_conf.get_conf()
        ban_score = int(100 * prj['pool_mixer']['random_accuracy'])
        filters.extend([
            RU_LANG_FILTER,
            rkub_filter(),
            skill_filter(  # Нет смысла давать проходить экзамен, если в боевой пул его уже не пустят
                prj['price_skill_id'],
                {"EQ": None, "GTE": ban_score}
            )
        ])

        owner = self.prj_conf.get_owner()
        my_skill_id = self.layout_conf.get_skill_id(owner)
        for l in self.prj_conf.list_used_layouts():
            if l.get_skill_id(owner) == my_skill_id:
                filters.extend([
                    # Исполнитель не показал уже себя с худшей стороны
                    skill_filter(my_skill_id, {"EQ": None,
                                               "LT": self.layout_conf.get_passing_score()}),
                    # Но и не прошёл ещё экзамен
                    skill_filter(my_skill_id, {"EQ": None,
                                               "GTE": self.layout_conf.get_ban_score()}),
                ])
            else:
                filters.append(
                    # Нет других проваленных экзаменов
                    skill_filter(l.get_skill_id(owner),
                                 {"EQ": None,
                                  "GTE": l.get_passing_score()}))

        return props

    def upload_hints(self, intent2hint=None):
        if intent2hint is None:
            intent2hint = self.prj_conf.get_merged_tree().map_intents_to_path()
        suites = self.layout_conf.list_training_suites(intent2hint)
        suites = [[{'input_values': {'dialog': t['dialog']},
                    'hint': t.get('hint'),
                    'solution': {'intent': t['intent']} if t.get('intent') else None}
                   for t in s]
                  for s in suites]

        self.upload_task_suites(suites, infinite_overlap=True)

