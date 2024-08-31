#!/usr/bin/env python
# encoding: utf-8
import re
from os.path import join
import logging

from utils.serializers import yaml
from utils.toloka.api import Requester

from exc import ValidationError
from tree import LayoutIntentTree
from parse_conf import Conf


class LayoutConf(Conf):
    CONF_TYPE = 'layouts'

    def get_skill_id(self, owner):
        ids = self.get_conf()['skill_id']
        if ids is None:
            return None
        elif isinstance(ids, dict):
            return str(ids[owner])
        else:
            return str(ids)  # FIXME: Старая нотация, удалить ветку после полной миграции

    def get_passing_score(self):
        # Выше какого уровня экзамен считается пройденным
        return self.get_conf().get('passing_score', 65)

    def get_ban_score(self):
        # Ниже какого уровня пользователь теряет шанс пройти экзамен
        return self.get_conf().get('ban_score', 33)

    def create_skill(self, toloka_env, owner):
        skill_id = self.get_skill_id(owner)
        if skill_id is not None:
            raise UserWarning('Skill already created id=%s' % skill_id)

        title = self.get_conf()['title']
        if title is None:
            raise UserWarning('Set title in conf.yaml!')

        r = Requester(toloka_env=toloka_env)
        props = {
            "name": title,
            "private_comment": 'Auto generated for Intent Layout "%s"' % self.key,
            "hidden": True,
            "training": False,
        }

        response = r.send('skills', props)
        ids = self.get_conf()['skill_id']
        if ids is None:
            self.get_conf()['skill_id'] = ids = {}
        ids[owner] = response['id']

        self.save_conf_on_disk()

        return response

    def get_yaml_tree(self):
        with open(join(self.get_conf_dir(), 'tree.yaml')) as inp:
            return yaml.load(inp)

    # Parse hints

    def list_training_suites(self, intent2hint):
        # Формат должен совпадать с выдачей из LayoutPool.list_training_suites()
        suite_list = []
        suite = []
        for task in self.iter_gs_tsv():
            if task is None:
                if suite:
                    suite_list.append(suite)
                    suite = []
                continue

            if not task['hint']:
                intent = task['intent']
                if intent is not None:
                    task['hint'] = u' -> '.join(intent2hint[intent])

            suite.append(task)

        if suite:
            suite_list.append(suite)

        return suite_list

    def iter_gs_tsv(self, filename='hints.tsv'):
        with open(join(self.get_conf_dir(), filename)) as inp:
            for line in inp:
                line = unicode(line.strip(), 'utf-8')
                if not line:
                    yield None  # Пустые тоже отдаём, т.к. они могут быть важны
                elif not line.startswith('INPUT:'):
                    yield self.parse_gs_line(line)

    def count_hints(self):
        return sum(1 for hint in self.iter_gs_tsv()
                   if hint and hint['intent'])

    split_comma = re.compile(r'(?<!\\),').split

    def parse_gs_line(self, line):
        parts = line.split('\t')
        dialog = [phrase.replace('\\,', ',')
                  for phrase in self.split_comma(parts[0].strip())]

        if len(parts) == 1:
            return {'dialog': dialog, 'intent': None, 'hint': None}

        intent = parts[1].strip()

        if len(parts) == 2:
            hint = ''  # При надобности будет проставлен из intent2path
        else:
            hint = parts[2].strip()

        return {'dialog': dialog, 'intent': intent, 'hint': hint}

    def intents_in_gs(self, filename='hints.tsv'):
        intents = set()
        for task in self.iter_gs_tsv(filename):
            if task is not None:
                intents.add(task['intent'])

        intents.discard(None)
        return intents

    MIN_GS_COUNT = 30

    def list_gs(self, all_known_intents=None):
        """
        :param Collection all_known_intents: Если указан, то будет проведена валидация, что все интенты включены в этом множество
        :return:
        """
        hp = [task
              for task in self.iter_gs_tsv('honeypots.tsv')
              if task and task['intent']]

        hints = [task
                 for task in self.iter_gs_tsv('hints.tsv')
                 if task and task['intent']]

        tree = LayoutIntentTree(self)
        known_intents = tree.map_intents_to_path()

        all_gs = hp + hints

        for task in all_gs:
            if task['intent'] not in known_intents:
                if all_known_intents is None or task['intent'] not in all_known_intents:
                    msg = 'There are no intent "{}" (from layout "{}") in whole project tree'.format(
                        task['intent'], self.key
                    )
                    raise ValidationError(msg)
                else:
                    msg = 'intent "{}" from layout "{}" is only in other layout tree'.format(
                        task['intent'], self.key
                    )
                    logging.warning(msg)

        if len(hp) < self.MIN_GS_COUNT:
            return all_gs
        else:
            return hp
