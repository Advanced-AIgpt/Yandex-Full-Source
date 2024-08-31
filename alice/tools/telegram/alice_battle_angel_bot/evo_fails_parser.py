import json
import logging
import os
import random
import re
import requests
import argparse

from .common import (ApiHandlers, get_latest_vins_release_st_ticket, get_st_ticket_comments, get_comment_about_evo_tests,
                        get_latest_evo_tests_sandbox_task_id, get_sandbox_task_context, get_staff_tg_logins,
                        get_staff_dismissed_logins, get_staff_absent_logins)


FOOD_EMOJI_MAP = {
    u':avocado:': u'\U0001F951',
    u':baby_bottle:': u'\U0001F37C',
    u':bacon:': u'\U0001F953',
    u':bagel:': u'\U0001F96F',
    u':baguette_bread:': u'\U0001F956',
    u':banana:': u'\U0001F34C',
    u':beer_mug:': u'\U0001F37A',
    u':bento_box:': u'\U0001F371',
    u':beverage_box:': u'\U0001F9C3',
    u':birthday_cake:': u'\U0001F382',
    u':bottle_with_popping_cork:': u'\U0001F37E',
    u':bowl_with_spoon:': u'\U0001F963',
    u':bread:': u'\U0001F35E',
    u':broccoli:': u'\U0001F966',
    u':burrito:': u'\U0001F32F',
    u':butter:': u'\U0001F9C8',
    u':candy:': u'\U0001F36C',
    u':canned_food:': u'\U0001F96B',
    u':carrot:': u'\U0001F955',
    u':cheese_wedge:': u'\U0001F9C0',
    u':cherries:': u'\U0001F352',
    u':chestnut:': u'\U0001F330',
    u':chocolate_bar:': u'\U0001F36B',
    u':clinking_beer_mugs:': u'\U0001F37B',
    u':clinking_glasses:': u'\U0001F942',
    u':cocktail_glass:': u'\U0001F378',
    u':coconut:': u'\U0001F965',
    u':cooked_rice:': u'\U0001F35A',
    u':cookie:': u'\U0001F36A',
    u':cooking:': u'\U0001F373',
    u':croissant:': u'\U0001F950',
    u':cucumber:': u'\U0001F952',
    u':cupcake:': u'\U0001F9C1',
    u':cup_with_straw:': u'\U0001F964',
    u':curry_rice:': u'\U0001F35B',
    u':custard:': u'\U0001F36E',
    u':cut_of_meat:': u'\U0001F969',
    u':dango:': u'\U0001F361',
    u':doughnut:': u'\U0001F369',
    u':dumpling:': u'\U0001F95F',
    u':ear_of_corn:': u'\U0001F33D',
    u':egg:': u'\U0001F95A',
    u':eggplant:': u'\U0001F346',
    u':falafel:': u'\U0001F9C6',
    u':fish_cake_with_swirl:': u'\U0001F365',
    u':fortune_cookie:': u'\U0001F960',
    u':french_fries:': u'\U0001F35F',
    u':fried_shrimp:': u'\U0001F364',
    u':garlic:': u'\U0001F9C4',
    u':glass_of_milk:': u'\U0001F95B',
    u':grapes:': u'\U0001F347',
    u':green_apple:': u'\U0001F34F',
    u':green_salad:': u'\U0001F957',
    u':hamburger:': u'\U0001F354',
    u':honey_pot:': u'\U0001F36F',
    u':hot_beverage:': u'\U00002615',
    u':hot_dog:': u'\U0001F32D',
    u':hot_pepper:': u'\U0001F336',
    u':ice_cream:': u'\U0001F368',
    u':kiwi_fruit:': u'\U0001F95D',
    u':leafy_green:': u'\U0001F96C',
    u':lemon:': u'\U0001F34B',
    u':lollipop:': u'\U0001F36D',
    u':mango:': u'\U0001F96D',
    u':mate:': u'\U0001F9C9',
    u':meat_on_bone:': u'\U0001F356',
    u':melon:': u'\U0001F348',
    u':moon_cake:': u'\U0001F96E',
    u':mushroom:': u'\U0001F344',
    u':oden:': u'\U0001F362',
    u':onion:': u'\U0001F9C5',
    u':oyster:': u'\U0001F9AA',
    u':pancakes:': u'\U0001F95E',
    u':peach:': u'\U0001F351',
    u':peanuts:': u'\U0001F95C',
    u':pear:': u'\U0001F350',
    u':pie:': u'\U0001F967',
    u':pineapple:': u'\U0001F34D',
    u':pizza:': u'\U0001F355',
    u':popcorn:': u'\U0001F37F',
    u':pot_of_food:': u'\U0001F372',
    u':potato:': u'\U0001F954',
    u':poultry_leg:': u'\U0001F357',
    u':pretzel:': u'\U0001F968',
    u':red_apple:': u'\U0001F34E',
    u':rice_ball:': u'\U0001F359',
    u':rice_cracker:': u'\U0001F358',
    u':roasted_sweet_potato:': u'\U0001F360',
    u':salt:': u'\U0001F9C2',
    u':sake:': u'\U0001F376',
    u':sandwich:': u'\U0001F96A',
    u':shallow_pan_of_food:': u'\U0001F958',
    u':shaved_ice:': u'\U0001F367',
    u':shortcake:': u'\U0001F370',
    u':soft_ice_cream:': u'\U0001F366',
    u':spaghetti:': u'\U0001F35D',
    u':steaming_bowl:': u'\U0001F35C',
    u':strawberry:': u'\U0001F353',
    u':stuffed_flatbread:': u'\U0001F959',
    u':sushi:': u'\U0001F363',
    u':taco:': u'\U0001F32E',
    u':takeout_box:': u'\U0001F961',
    u':tangerine:': u'\U0001F34A',
    u':teacup_without_handle:': u'\U0001F375',
    u':tomato:': u'\U0001F345',
    u':tropical_drink:': u'\U0001F379',
    u':tumbler_glass:': u'\U0001F943',
    u':waffle:': u'\U0001F9C7',
    u':watermelon:': u'\U0001F349',
    u':wine_glass:': u'\U0001F377',
    u':eggplant:': u'\U0001F346',
    u':baby_bottle:': u'\U0001F37C',
    u':banana:': u'\U0001F34C',
    u':bottle_with_popping_cork:': u'\U0001F37E',
    u':bread:': u'\U0001F35E',
    u':burrito:': u'\U0001F32F',
    u':candy:': u'\U0001F36C',
    u':cheese_wedge:': u'\U0001F9C0',
    u':cherries:': u'\U0001F352',
    u':chestnut:': u'\U0001F330',
    u':chocolate_bar:': u'\U0001F36B',
    u':cookie:': u'\U0001F36A',
    u':custard:': u'\U0001F36E',
    u':dango:': u'\U0001F361',
    u':doughnut:': u'\U0001F369',
    u':fried_shrimp:': u'\U0001F364',
    u':grapes:': u'\U0001F347',
    u':green_apple:': u'\U0001F34F',
    u':hamburger:': u'\U0001F354',
    u':honey_pot:': u'\U0001F36F',
    u':hot_dog:': u'\U0001F32D',
    u':hot_pepper:': u'\U0001F336',
    u':ice_cream:': u'\U0001F368',
    u':lemon:': u'\U0001F34B',
    u':lollipop:': u'\U0001F36D',
    u':meat_on_bone:': u'\U0001F356',
    u':melon:': u'\U0001F348',
    u':mushroom:': u'\U0001F344',
    u':oden:': u'\U0001F362',
    u':peach:': u'\U0001F351',
    u':pear:': u'\U0001F350',
    u':pineapple:': u'\U0001F34D',
    u':popcorn:': u'\U0001F37F',
    u':poultry_leg:': u'\U0001F357',
    u':rice_ball:': u'\U0001F359',
    u':rice_cracker:': u'\U0001F358',
    u':sake:': u'\U0001F376',
    u':shaved_ice:': u'\U0001F367',
    u':pizza:': u'\U0001F355',
    u':spaghetti:': u'\U0001F35D',
    u':strawberry:': u'\U0001F353',
    u':sushi:': u'\U0001F363',
    u':taco:': u'\U0001F32E',
    u':tangerine:': u'\U0001F34A',
    u':tomato:': u'\U0001F345',
    u':tropical_drink:': u'\U0001F379',
    u':watermelon:': u'\U0001F349',
    u':wine_glass:': u'\U0001F377',
}


FROST_EMOJI_MAP = {
    u':Santa_Claus_light_skin_tone:': u'\U0001F385\U0001F3FB',
    u':cloud_with_snow:': u'\U0001F328',
    u':cold_face:': u'\U0001F976',
    u':ice:': u'\U0001F9CA',
    u':snow_capped_mountain:': u'\U0001F3D4',
    u':snowflake:': u'\U00002744',
    u':snowman:': u'\U00002603',
    u':snowman_without_snow:': u'\U000026C4',
}


class EvoFailsParser(object):

    ALICE_PATH = 'alice/tests/integration_tests'

    def __init__(self, update, raw=False):
        self._blame = dict()
        self._blob = dict()
        self._node = dict()
        self._raw = raw

        args = self._parse_args(update.message.text.split())
        self._custom_task_id = args.task_id
        self._only_release = args.release

    def _parse_args(self, argv):
        parser = argparse.ArgumentParser()
        parser.add_argument('task_id', nargs='?', type=int)
        parser.add_argument('-r', '--release', action='store_true')
        return parser.parse_args(argv[1:])

    @staticmethod
    def _is_good(line):
        return line.startswith('[<span style="color:#457b23;">good</span>]')

    @staticmethod
    def _is_bad(line):
        return line.startswith('[<span style="color:#b7141e;">fail</span>]')

    @staticmethod
    def _get_report_content(task_context):
        if 'report' in task_context:
            return task_context['report']

        results_url = task_context['html_result_path']
        return requests.get(results_url).text

    def _parse_runs_lines(self):
        started = False
        runs = []
        cur_lines = []

        for line in self._report.split('\n'):
            if self._is_bad(line) or self._is_good(line):
                if not started:
                    started = True
                if cur_lines:
                    runs.append(cur_lines)
                cur_lines = [line]
            else:
                if started:
                    cur_lines.append(line)
        if cur_lines:
            runs.append(cur_lines)
        return runs

    def _parse_bad_names(self, runs):
        names = []
        release_bad_names = set()
        for run in runs:
            if self._is_bad(run[0]):
                m = re.match(r'\[.*fail.*\] <span .*>(?P<full_name>(?P<class_name>.*?)<\/span>::(?P<method_name>.*?)\[.*?\]) .*', run[0])
                names.append((m['class_name'], m['method_name'], m['full_name'].replace('</span>', '')))

                for line in run:
                    if 'ReleaseBugError' in line:
                        release_bad_names.add(m['full_name'].replace('</span>', ''))

        return names, release_bad_names

    @staticmethod
    def _parse_file_path(class_name):
        file_name = class_name
        if '::' in file_name:
            file_name = file_name[:file_name.find('::')]
        file_name = file_name.replace('.', '/')
        if file_name.endswith('/py'):
            file_name = file_name[:-3] + '.py'
        return 'alice/tests/integration_tests/{}'.format(file_name)

    # --- blame helpers ---
    def _get_blame_raw(self, path):
        if path not in self._blame:
            url = ApiHandlers.ARCANUM + '/blame' + '/trunk/arcadia/' + path
            resp = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['ARCANUM_TOKEN']})
            self._blame[path] = resp.text
        return self._blame[path]

    def _get_blame(self, path):
        blame = json.loads(self._get_blame_raw(path))
        res = dict()
        for line in blame:
            res[line['lineNumber']] = line
        return res

    # --- blob helpers ---
    def _get_blob(self, path):
        if path not in self._blob:
            url = ApiHandlers.ARCANUM + '/blob' + '/trunk/arcadia/' + path
            resp = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['ARCANUM_TOKEN']})
            self._blob[path] = resp.text
        return self._blob[path]

    # --- node helpers ---
    def _get_node(self, path):
        if path not in self._node:
            url = ApiHandlers.ARCANUM + '/node' + '/trunk/arcadia/' + path
            resp = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['ARCANUM_TOKEN']})
            self._node[path] = json.loads(resp.text)
        return self._node[path]

    # --- owners cache helper ---
    def _get_children_files(self, root):
        logging.info('Getting all python files in root %s' % root)
        res = []

        def dfs(cur_path):
            node = self._get_node(cur_path)
            if node['type'] == 'dir':
                for sub_node in node['children']:
                    dfs(cur_path + '/' + sub_node['name'])
            else:
                res.append(cur_path)

        dfs(root)
        return res

    def _parse_classes(self, path):
        logging.info('Working with path %s' % path)

        lines = self._get_blob(path).split('\n')

        path_code = path[len(self.ALICE_PATH)+1:].replace('/', '.')

        result = []
        def add_class(classname, parents, owners):
            if classname:
                result.append((classname, parents, owners, path_code))

        cur_classname = None
        cur_parents = None
        cur_owners = None

        for line in lines:
            m = re.search(r'^class (?P<name>.*?)(\((?P<parent>.*)?\))?:', line)
            if m:
                add_class(cur_classname, cur_parents, cur_owners)
                cur_classname = m['name']
                if 'parent' in m.groupdict() and m['parent']:
                    cur_parents = [s.strip() for s in m['parent'].split(',')]
                    cur_parents = [s[s.rfind('.')+1:] for s in cur_parents if s != 'object']
            else:
                m = re.search(r'owners = \((?P<owners>.*)\)', line)
                if m:
                    cur_owners = [s.strip().strip('\'') for s in m['owners'].split(',')]
                    cur_owners = [s for s in cur_owners if s]

        add_class(cur_classname, cur_parents, cur_owners)
        return result

    def _find_parent_owner(self, classes, root):
        for classname, parents, owners, _ in classes:
            if classname == root:
                if owners:
                    return owners
                if parents:
                    maybe_owners = None
                    for parent in parents:
                        if parent != root:
                            maybe_owners = self._find_parent_owner(classes, parent)
                            if maybe_owners:
                                return maybe_owners
        return None

    def _build_owners_cache(self):
        files = self._get_children_files(self.ALICE_PATH)
        py_files = [f for f in files if f.endswith('.py')]

        classes = []
        for py_file in py_files:
            classes.extend(self._parse_classes(py_file))

        owners_dict = dict()
        for classname, parents, owners, path_code in classes:
            if (not owners) and parents:
                for parent in parents:
                    owners = self._find_parent_owner(classes, parent)
                    if owners:
                        break
            owners_dict[path_code + '::' + classname] = owners

        return owners_dict

    # --- other helpers ---
    @staticmethod
    def _get_spaces_at_start(line):
        if not line:
            return 0
        res = 0
        while res < len(line) and line[res] == ' ':
            res += 1
        return res

    # --- format raw Telegram digest ---
    def _form_raw_telegram_message(self, result, telegram_logins):
        counter = {}
        for _, author in result:
            author_hash = json.dumps(author)
            if author_hash not in counter:
                counter[author_hash] = 0
            counter[author_hash] += 1
        sorted_list = sorted([author_hash for author_hash in counter])

        msg = ''
        for author_hash in sorted_list:
            author = json.loads(author_hash)
            if not author:
                msg += 'неизвестный автор, или в trunk такого класса нет '
            else:
                def link(author_name):
                    s = '{}'.format(author_name.replace('_', '\\_'))
                    return s
                msg += ', '.join([link(author_name) for author_name in author])
            msg += ':\n'
            for full_name, real_author in result:
                if author == real_author:
                    msg += '`{}`\n'.format(full_name)
            msg += '\n'

        return msg

    # --- format Telegram digest ---
    def _form_telegram_message(self, full_result, release_bad_names, telegram_logins):
        msg = ''

        msg += 'Привет! Это автоматически сгенерированное сообщение от бота команды инфраструктуры Алисы.\n\n'

        msg += 'Я посмотрел результаты тестирования *{}* на {}\n\n'.format(self._vins_version, 'https://sandbox.yandex-team.ru/task/{}/error\\_report'.format(self._task_id))

        msg += 'К сожалению, часть тестов упала. Я автоматически определил наших коллег, которые, возможно, смогут их посмотреть.\n\n'

        msg += 'Я определил это по полю `owners` в тестах.\n'

        msg += 'Нам нужна ваша помощь в фиксах тестов. Помогите нам пройти приемку по evo-тестам! 😁\n\n'

        msg += 'Количество сломанных тестов: *{}*\n\n'.format(len(full_result))

        for about_bad_names in [True] if self._only_release else [True, False]:
            result = []
            for full_name, author in full_result:
                if (full_name in release_bad_names) == about_bad_names:
                    result.append((full_name, author))

            if about_bad_names:
                msg += '*Релизные падения : {}*'.format(len(result))
            else:
                msg += '*Нерелизные падения : {}*'.format(len(result))

            unique_logins = set()

            counter = {}
            for _, author in result:
                author_hash = json.dumps(author)
                if author_hash not in counter:
                    counter[author_hash] = 0
                counter[author_hash] += 1
                authors_json = json.loads(author_hash) or []
                for author in authors_json:
                    if not author.startswith('g:'):
                        unique_logins.add(author)
            sorted_list = reversed(sorted([(counter[author_hash], author_hash) for author_hash in counter]))

            unique_logins = list(unique_logins)
            dismissed_logins = get_staff_dismissed_logins(unique_logins)

            try:
                absent_logins = get_staff_absent_logins(unique_logins)
            except:
                logging.error('Can\'t get absent staff logins')
                absent_logins = {}

            msg += '\n\n'
            sorted_list = reversed(sorted([(counter[author_hash], author_hash) for author_hash in counter]))
            for count, author_hash in sorted_list:
                author = json.loads(author_hash) or []
                if not author:
                    msg += 'неизвестный автор, или в trunk такого класса нет '
                else:
                    def link(author_name):
                        if author_name.startswith('g:'):
                            s = '[группа {}](https://a.yandex-team.ru/arc/trunk/arcadia/groups/{})'.format(author_name, author_name[2:])
                        elif author_name.startswith('abc:'):
                            s = '[группа {}](https://abc.yandex-team.ru/services/{})'.format(author_name, author_name[4:])
                        else:
                            s = '[{}](https://staff.yandex-team.ru/{})'.format(author_name.replace('_', '\\_'), author_name.replace('_', '\\_'))
                            if author_name in dismissed_logins:
                                s += ' \\[*Уволился*] '
                            elif author_name in absent_logins:
                                reason = absent_logins[author_name]
                                if reason == 'Дежурство':
                                    s += ' \\[@{}] '.format(telegram_logins[author_name].replace('_', '\\_'))
                                else:
                                    s += ' \\[`@{}`] '.format(telegram_logins[author_name].replace('_', '\\_'))
                                    s += '\\[*{}*] '.format(reason)
                            else:
                                s += ' \\[@{}] '.format(telegram_logins[author_name].replace('_', '\\_'))
                        return s
                    msg += ', '.join([link(author_name) for author_name in author])
                msg += ': *{}* '.format(count)

                emoji_map = FOOD_EMOJI_MAP if all(author_name not in dismissed_logins for author_name in author) else FROST_EMOJI_MAP
                while count >= 1:
                    msg += emoji_map[random.choice(list(emoji_map.keys()))]
                    count -= 1
                msg += '\n'

                for full_name, real_author in result:
                    if author == real_author or (not author and not real_author):
                        msg += '`{}`\n'.format(full_name)
                msg += '\n\n'

        return msg

    # --- main message preparer method ---
    def _prepare_msg(self):
        owners_cache = self._build_owners_cache()

        runs = self._parse_runs_lines()
        bad_names, release_bad_names = self._parse_bad_names(runs)

        result = []
        authors = []

        for class_name, method, full_name in bad_names:
            author = []
            if class_name in owners_cache:
                author = owners_cache[class_name]
                authors.extend(author or [])
            result.append((full_name, author))

        telegram_logins = get_staff_tg_logins(authors)
        if self._raw:
            return self._form_raw_telegram_message(result, telegram_logins)
        else:
            return self._form_telegram_message(result, release_bad_names, telegram_logins)

    # --- main parse method ---
    def parse(self):
        if self._custom_task_id:
            report = self._get_report_content(get_sandbox_task_context(self._custom_task_id))

            self._report = report
            self._vins_version = '{}'.format('(по кастомному запуску)')
            self._task_id = self._custom_task_id
        else:
            ticket = get_latest_vins_release_st_ticket(count=1)[0]
            comments = get_st_ticket_comments(ticket['key'])
            evo_tests_comment = get_comment_about_evo_tests(comments)
            evo_tests_task_id, minor_version = get_latest_evo_tests_sandbox_task_id(evo_tests_comment)
            report = self._get_report_content(get_sandbox_task_context(evo_tests_task_id))

            self._report = report
            self._vins_version = '{}-{}'.format(ticket['summary'], minor_version)
            self._task_id = evo_tests_task_id

        return self._prepare_msg()
