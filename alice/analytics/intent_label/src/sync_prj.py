#!/usr/bin/env python
# encoding: utf-8
"""
Синхронизация конфигов с Толокой
"""
import logging

from utils.json_utils import json_dumps
from utils.toloka.api import ProjectRequester

from exc import ValidationError
from parse_prj import ProjectConf
from sync_layout import LayoutPool


class IntentProjectRequester(ProjectRequester):

    def __init__(self, prj_key, toloka_env=None, **kwargs):
        self.prj_key = prj_key
        self.prj_conf = ProjectConf(prj_key)
        # аргумент toloka_env игнорируется потому что вычисляется из настроек проекта
        super(IntentProjectRequester, self).__init__(prj_id=self.prj_conf.get_id(),
                                                     toloka_env=self.prj_conf.toloka_env(),
                                                     **kwargs)

    def get_skill_pools(self, exclude_skill_id=None):
        skills = {}
        for p in self.list_open_pools():
            rules = p.get('quality_control')
            if rules is None:
                continue

            for conf in rules['configs']:
                for rule in conf['rules']:
                    if rule['action']['type'] == 'SET_SKILL_FROM_OUTPUT_FIELD':
                        skill_id = rule['action']['parameters']['skill_id']
                        if exclude_skill_id == skill_id:
                            break
                        if skill_id in skills:
                            msg = 'Duplicated pools ({pool1}, {pool2}) for skill {skill}'
                            raise ValidationError(msg.format(skill=skill_id, pool1=skills[skill_id], pool2=p['id']))
                        skills[skill_id] = p['id']
                        break
        return skills

    def sync_skill_pools(self):
        skill_pools = self.get_skill_pools(
            exclude_skill_id=self.prj_conf.get_conf()['price_skill_id']
        )
        layouts = self.prj_conf.list_used_layouts()
        intent2hint = self.prj_conf.get_merged_tree().map_intents_to_path()
        for l in layouts:
            self.sync_one_layout(l, skill_pools, intent2hint)

        if skill_pools:
            logging.warning('Skills and pools without layout connected {}'.format(skill_pools))
            # TODO: ссылки на пулы и скилы
            # TODO: Может принудительно закрывать?

    def sync_one_layout(self, layout, skill_pools, intent2hint):
        skill_id = layout.get_skill_id(self.prj_conf.get_owner())
        pool_id = skill_pools.pop(skill_id, None)
        if pool_id is None:
            lp = self.pool_for_layout(layout_conf=layout, pool_id=None)
            lp.create_pool()
            logging.info('Create exam pool for layout "{}", skill_id={}, pool_id={}'.format(
                layout.key, skill_id, lp.pool_id)
            )
            lp.upload_hints(intent2hint)
            lp.open_pool()
        else:
            r = self.pool_for_layout(layout_conf=layout, pool_id=pool_id)
            # print '======', skill_id, r.prj_id, r.pool_id, l.key
            if r.list_training_suites() == layout.list_training_suites(intent2hint):
                logging.info('Keep exam pool {} for layout "{}"'.format(
                    pool_id, layout.key
                ))
                # TODO: Проверять и другие свойства? (доступы, уровни прохождения и т.д.)
            else:
                r.close_pool()

                lp = self.pool_for_layout(layout_conf=layout, pool_id=None)
                lp.create_pool()
                logging.info('Replace exam pool for layout "{}", {} -> {}'.format(
                    layout.key, pool_id, r.pool_id)
                )
                lp.upload_hints(intent2hint)
                lp.open_pool()

    def pool_for_layout(self, layout_conf, pool_id):
        return LayoutPool(layout_conf=layout_conf, prj_conf=self.prj_conf,
                          pool_id=pool_id,
                          toloka_env=self.toloka_env, token=self.token)

    def sync_prj_props(self):
        def patch_fn(props):
            logging.info('Old project properties: {}'.format(json_dumps(props)))
            mt = self.prj_conf.get_merged_tree()
            # TODO: сверку размера дерева с актуальным шаблоном, проверка консистентности шаблона
            view_spec = props['task_spec']['view_spec']
            view_spec['script'] = mt.to_script()
            view_spec['styles'] = self.prj_conf.get_spec_content('styles.css')
            view_spec['markup'] = self.prj_conf.get_spec_content('markup.html')

            old_instr = props['public_instructions']
            new_instr = mt.to_instruction(old_instr)
            if new_instr is not None:
                props['public_instructions'] = new_instr

        self.patch_prj(patch_fn)

    def check_layout_skill_ids(self):
        used = {}
        for l in self.prj_conf.list_used_layouts():
            skill_id = l.get_skill_id(self.prj_conf.get_owner())
            if skill_id is None:
                raise ValidationError('Create and store skill for layout "{}"!'.format(l.key))
            if skill_id in used:
                raise ValidationError(
                    'Duplicated skill_id {} for layouts "{}" and "{}"'.format(
                        skill_id, used[skill_id], l.key
                    )
                )
            used[skill_id] = l.key
