import json
import logging
import os
import re
import requests
from requests import HTTPError
from urllib.parse import quote


SANDBOX_TASK_TEMPLATE = 'template/evo_sandbox_task_template.json'


class ApiHandlers(object):
    ABC = 'https://abc-back.yandex-team.ru/api/v4/duty/on_duty/'
    ARCANUM = 'https://a.yandex-team.ru/api/tree'
    SANDBOX = 'https://sandbox.yandex-team.ru/api/v1.0'
    STAFF = 'https://staff-api.yandex-team.ru/v3'
    GAP = 'https://staff.yandex-team.ru/'
    STARTREK = 'https://st-api.yandex-team.ru/v2'
    UNIPROXY = 'wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws'


def create_sandbox_task_from_template(ticket, test_id):
    """
        Returns new Sandbox task_id
    """
    with open(SANDBOX_TASK_TEMPLATE) as t:
        sb_task_params = json.load(t)

    sb_task_params['description'] = 'EVO tests from {} ticket, test_id: {}'.format(ticket, test_id)
    sb_task_params['custom_fields'][0]['value'] = ticket
    sb_task_params['custom_fields'][1]['value'] = ApiHandlers.UNIPROXY + '?test-id=' + test_id
    logging.debug("Creating SB task with parameters: {}".format(sb_task_params))
    url = ApiHandlers.SANDBOX + '/task'
    try:
        response = requests.post(url, json=sb_task_params, headers={'Authorization': 'OAuth ' + os.environ['SANDBOX_TOKEN']})
        response.raise_for_status()
    except HTTPError:
        logging.error('Can not create Sandbox task')
        return None
    return json.loads(response.text)['url'].split('/')[6]


def get_latest_vins_release_st_ticket(count=1):
    """
        Returns dict about the StarTrek ticket of the latest VINS release
    """
    url = ApiHandlers.STARTREK + '/issues?filter=queue:ALICERELEASE&filter=summary:VINS&filter=resolution:empty()'
    response = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['STARTREK_TOKEN']})

    data = json.loads(response.text)
    if not data:
        return None

    res = []
    for val in reversed(sorted(data, key=lambda val: val['summary'])):
        res.append(val)
        if len(res) == count:
            break
    return res


def get_st_ticket(key):
    url = ApiHandlers.STARTREK + '/issues/' + key
    response = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['STARTREK_TOKEN']})
    logging.debug('StartTrek code: {}'.format(response.status_code))
    logging.debug('StartTrek response: "{}"'.format(response.text))
    return json.loads(response.text)


def get_st_ticket_comments(key):
    """
        Returns list of comments from StarTrek ticket
    """
    url = ApiHandlers.STARTREK + '/issues/' + key + '/comments'
    response = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['STARTREK_TOKEN']})
    return json.loads(response.text)


def get_comment_about_evo_tests(comments):
    def about_evo_tests(comm):
        return (comm['createdBy']['id'] == 'robot-bassist') and ('Alice EVO Integration Tests' in comm['text'])

    return next((comm for comm in comments if about_evo_tests(comm)), None)


def get_comment_about_manual_testing(comments):
    def about_manual_testing(comm):
        return 'Сводный комментарий тестирования' in comm['text']

    return next((comm for comm in comments if about_manual_testing(comm)), None)


def get_test_ids(ticket):
    desc = ticket['description']
    test_ids = re.findall(r'\[\*\*(\d+)\*\*\]', desc, flags=re.DOTALL)
    if test_ids:
        logging.info('Test IDs found: "%s"' % test_ids)
        return test_ids
    logging.error('Unable to parse ticket')


def get_latest_evo_tests_sandbox_task_id(evo_tests_comment):
    text = evo_tests_comment['text']
    test_reports_tasks = re.findall(r'Error test report: https://sandbox.yandex-team.ru/task/(.*?)/error_report\n', text)

    return int(test_reports_tasks[-1]), len(test_reports_tasks)


def get_sandbox_task_context(task_id):
    url = ApiHandlers.SANDBOX + '/task/' + str(task_id) + '/context'
    response = requests.get(url=url)
    return json.loads(response.text)


def get_staff_dismissed_logins(users):
    query = '/persons?official.is_dismissed=true&login=' + quote(','.join(users)) + '&_fields=login'
    response = requests.get(url=ApiHandlers.STAFF + query, headers={'Authorization': 'OAuth ' + os.environ['ARCANUM_TOKEN']})

    logins = set()
    staff = json.loads(response.text)
    for elem in staff.get('result', []):
        login = elem.get('login')
        if login is not None:
            logins.add(login)
    return logins


def get_staff_absent_logins(users):
    query = '/gap-api/api/workflows/'
    response = requests.get(url=ApiHandlers.GAP + query, headers={'Authorization': 'OAuth ' + os.environ['ARCANUM_TOKEN']})
    workflows = json.loads(response.text)
    workflows_desc = {}
    for wf in workflows['workflows']:
        workflows_desc[wf['type']] = wf['verbose_name']['ru']

    query = '/gap-api/old_api/current.xml?ext=true&login_list=' + quote(','.join(users))
    response = requests.get(url=ApiHandlers.GAP + query, headers={'Authorization': 'OAuth ' + os.environ['ARCANUM_TOKEN']})

    logins = {}

    from xml.etree import ElementTree as ET
    tree = ET.fromstring(response.text)
    for absent in tree[0]:
        attrib = absent.attrib
        logins[attrib['login']] = workflows_desc[attrib['subject_id']]
    return logins


def get_staff_tg_logins(users):
    users = set(users)
    query = '/persons?login=' + quote(','.join(users)) + '&_fields=' + quote(','.join(['login', 'accounts']))
    response = requests.get(url=ApiHandlers.STAFF + query, headers={'Authorization': 'OAuth ' + os.environ['ARCANUM_TOKEN']})

    staff = json.loads(response.text)
    telegram_logins = dict()

    for row in staff['result']:
        staff_login = row['login']
        telegram_login = None
        for account in row['accounts']:
            if account['type'] == 'telegram':
                telegram_login = account['value']
        telegram_logins[staff_login] = telegram_login

    return telegram_logins


def get_staff_info_by_tg_login(login):
    url = ApiHandlers.STAFF + '/persons'
    headers = {'Authorization': 'OAuth {0}'.format(os.environ['ARCANUM_TOKEN'])}
    staff_query = '{"type": "telegram", "value_lower": "%s"}' % login.lower()
    try:
        payload = {
            '_one': 1,
            '_query': 'accounts == match(%s) and official.is_dismissed == False' % staff_query
        }
        response = requests.get(url, headers=headers, params=payload)
        response.raise_for_status()
    except HTTPError:
        logging.info('Non Yandex worker detected: "%s"' % login)
        return None
    return response.json()


def get_release_duty_info():
    url = ApiHandlers.ABC + '?service=2651&fields=person.login,person.name'
    response = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['ABC_TOKEN']})

    info = json.loads(response.text)[0]
    return {'login': info['person']['login'], 'name': info['person']['name']['ru']}


def run_sandbox_task(task_id):
    url = ApiHandlers.SANDBOX + '/batch/tasks/start'
    try:
        response = requests.put(url, json=[task_id], headers={'Authorization': 'OAuth ' + os.environ['SANDBOX_TOKEN']})
        response.raise_for_status()
    except HTTPError:
        logging.error('Can not run task {}'.format(task_id))
        return None
    logging.info('task created: {}'.format(response.json()))
    return task_id


def slice_long_message(text):
    def utf8len(s):
        return len(s.encode('utf-8'))

    res = []
    cur = ''
    for line in text.split('\n'):

        # the limit is 4096 bytes per message accoding to documentation
        # but actually messages with length > 3000 bytes lead to error
        # when bot is hosted on YaDeploy (that does not reproduce locally)
        if utf8len(cur) + utf8len(line) + 1 < 2048:
            cur += line + '\n'
        else:
            res.append(cur)
            cur = line + '\n'
    if cur:
        res.append(cur)
    return res


def validate_st_ticket_name(text):
    text_split = text.split()
    if len(text_split) == 2 and re.match(r'^[A-Z]+-[0-9]+$', text_split[1]):
        return text_split[1]
    logging.error('Telegram message validation failed. {} - is not StarTrack ticket name'.format(text))


def build_duty_summon(text):
    duty_man = get_release_duty_info()
    tg_logins = get_staff_tg_logins(['avitella', 'ispetrukhin', duty_man['login']])

    text_split = text.split(maxsplit=1)

    duty_template = '🥾 Релиз-менеджер: [{}](https://staff.yandex-team.ru/{}) '
    if len(text_split) > 1:
        duty_template += '\\[@{}]\n'
    else:
        duty_template += '\\[`@{}`]\n'

    msg = duty_template.format(duty_man['login'].replace('_', '\\_'), duty_man['login'].replace('_', '\\_'), tg_logins[duty_man['login']].replace('_', '\\_'))

    oker_template = '⚖️  Для мержа в ветку нужен OK от: [{}](https://staff.yandex-team.ru/{})'
    if len(text_split) > 1:
        oker_template += '\\[@{}]'
    else:
        oker_template += '\\[`@{}`]'
    oker_template += ' (или `@avitella`)\n'
    msg += oker_template.format('ispetrukhin', 'ispetrukhin', tg_logins['ispetrukhin'])

    if len(text_split) > 1:
        msg += '\n Прошу ОКнуть мерж коммита: ' + text_split[1]

    return msg
