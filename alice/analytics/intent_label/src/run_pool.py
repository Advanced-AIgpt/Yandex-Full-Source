#!/usr/bin/env python
# encoding: utf-8
from string import Template
import json

from utils.nirvana.op_caller import get_current_workflow_url, get_current_workflow_title
from utils.toloka.api import PoolRequester
from utils.toloka.filters import RU_LANG_FILTER, rkub_filter, skill_filter
from utils.toloka.std_pool import AccuracyQualifier

from parse_conf import get_common_spec
from parse_prj import ProjectConf


class IntentPoolRequester(PoolRequester):
    def __init__(self, prj_conf, pool_id=None, **kwargs):
        if isinstance(prj_conf, basestring):
            prj_conf = ProjectConf(prj_conf)
        self.prj_conf = prj_conf
        self.layouts = prj_conf.list_used_layouts()
        self.pool_mixer = prj_conf.get_conf()['pool_mixer']
        self.acc_q = AccuracyQualifier(self.pool_mixer['random_accuracy'])
        super(IntentPoolRequester, self).__init__(
            prj_id=prj_conf.get_id(),
            toloka_env=prj_conf.toloka_env(),
            pool_id=pool_id,
            **kwargs
        )

    def list_honeypots(self):
        all_known_intents = self.prj_conf.get_merged_tree().map_intents_to_path()
        gs = []
        for l in self.layouts:
            for task in l.list_gs(all_known_intents):  # Здесь заодно происходит валидация всех хранимых хинтов и ханипотов
                gs.append({
                    "inputValues": {
                        "dialog": task['dialog'],
                    },
                    "knownSolutions": [{"outputValues": {"intent": task['intent']}}]
                })
        return gs

    def _make_pool_props(self):
        tmpl = Template(get_common_spec('base_pool_props.json'))
        prj = self.prj_conf.get_conf()
        url = get_current_workflow_url()
        mixer = self.pool_mixer
        props = json.loads(tmpl.substitute(
            prj_id=self.prj_id,
            private_name=get_current_workflow_title() or u'Разметка интентов',
            private_comment="Created from local script" if url is None else 'Created from %s' % url,
            price_skill_id=prj['price_skill_id'],
            ban_score=int(mixer['random_accuracy'] * 100),  # TODO: Из параметров
            page_size=mixer['page_size'],
            gs_count=mixer['gs_count'],
            overlap=mixer['overlap'],
            tasks_count=mixer['page_size'] - mixer['gs_count'],
            expire_date=self.ttl_to_expire(),
        ))
        props['dynamic_pricing_config']['intervals'] = list(
            self.acc_q.iter_price_intervals(self.prj_conf.get_price_intervals())
        )
        self._add_ban_rules(props)
        self._add_worker_filters(props)
        return props

    def _add_worker_filters(self, props):
        owner = self.prj_conf.get_owner()
        filters = props['filter']['and']
        filters.append(RU_LANG_FILTER)
        filters.append(rkub_filter())
        for l in self.layouts:
            filters.append(skill_filter(l.get_skill_id(owner),
                                        {'GTE': l.get_passing_score()}))
        return props

    def _add_ban_rules(self, props):
        gs_count = self.pool_mixer['gs_count']
        rules = props['quality_control']['configs']
        rules.append(self.acc_q.ban_rule(gs_count*2, 1))
        rules.append(self.acc_q.ban_rule(gs_count*4, 1.3))
        return props
