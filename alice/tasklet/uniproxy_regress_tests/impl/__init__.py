import datetime
import json
import logging
from urllib.parse import urljoin

import requests
from ci.tasklet.common.proto import service_pb2 as ci
from sandbox.sandboxsdk.channel import channel
from sandbox.sandboxsdk.errors import SandboxTaskFailureError
from startrek_client import Startrek
from tasklet.services.yav.proto import yav_pb2 as yav

from alice.tasklet.uniproxy_regress_tests.proto import uniproxy_regress_tests_tasklet

ALLOWED_ST_QUEUES = ['ALICERELEASE', 'ALICERELTEST']
ASSESSORS_QUEUE = 'ALICEASSESSORS'
ST_API_URL = 'https://st-api.yandex-team.ru/v2/issues/'
ST_UI_URL = 'https://st.yandex-team.ru/'
TESTPALM_API_URL = 'https://testpalm-api.yandex-team.ru/'

ALICEOPS_ST_TOKEN_KEY = 'st.token'

MAD_HATTER_GROUP = 'WONDERLAND'
MAD_HATTER_ST_TOKEN = 'env.STARTREK_OAUTH_TOKEN'
MAD_HATTER_TESTPALM_TOKEN = 'env.TESTPALM_OAUTH_TOKEN'

LOGGER = logging.getLogger('UniproxyRegressTestsTask')


class UniproxyRegressTestsImpl(uniproxy_regress_tests_tasklet.UniproxyRegressTestsBase):
    def run(self):
        release_service = 'uniproxy'
        alice_st_token = self.ctx.yav.get_secret(
            yav.YavSecretSpec(uuid=self.input.context.secret_uid, key=ALICEOPS_ST_TOKEN_KEY),
            default_key=ALICEOPS_ST_TOKEN_KEY
        ).secret

        release_ticket = self.input.config.st_ticket
        testsuite = self.input.config.testsuite
        try:
            st_token = channel.task.get_vault_data(MAD_HATTER_GROUP, MAD_HATTER_ST_TOKEN)
            testpalm_token = channel.task.get_vault_data(MAD_HATTER_GROUP, MAD_HATTER_TESTPALM_TOKEN)

            if not st_token or not testpalm_token:
                raise Exception('Testpalm and ST tokens are required')

            regress_ticket = create_regress(
                release_service,
                release_ticket,
                testsuite,
                st_token,
                testpalm_token,
                self.input.config.uniproxy_websocket_url,
            )
            progress_report = self._prepare_progress_update(ST_UI_URL, regress_ticket)
            self.ctx.ci.UpdateProgress(progress_report)

            post_comment_in_release_ticket(release_ticket, regress_ticket, alice_st_token)
            self.output.state.success = True
            self.output.state.ticket = regress_ticket
        except Exception as exc:
            LOGGER.exception('catch exception')
            raise SandboxTaskFailureError(str(exc))

    def _prepare_progress_update(self, st_url, ticket):
        startrek_url = urljoin(st_url, ticket)

        progress = ci.TaskletProgress()
        progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress.id = 'ST url'
        progress.text = "Regress ticket"
        progress.url = startrek_url
        progress.module = "STARTREK"
        progress.status = ci.TaskletProgress.Status.RUNNING
        return progress


def post_comment_in_release_ticket(release_ticket, regress_ticket, alice_st_token):
    client = Startrek(
        useragent="Voicetech CI tasklet",
        base_url=ST_API_URL,
        token=alice_st_token
    )
    issue = client.issues[release_ticket]
    comment_text = "Successfully created ticket for regress tests" + (
        '' if regress_ticket is None else ('\n' + regress_ticket))
    issue.comments.create(text=comment_text)


def create_regress(release, ticket, testsuite, st_token, testpalm_token, uniproxy_url):
    # Example: {'Authorization': 'OAuth testpalm_token', "Content-Type": "application/json"}
    testpalm_header = generate_headers(testpalm_token)

    # Example: {'Authorization': 'OAuth st_token', "Content-Type": "application/json"}
    st_header = generate_headers(st_token)

    # Example: https://testpalm-api.yandex-team.ru/testrun/alice/create/
    alice_run_suite_url = TESTPALM_API_URL + "testrun/" + testsuite + "/create/"

    # Example: https://testpalm-api.yandex-team.ru/testsuite/alice/
    alice_suite_url = TESTPALM_API_URL + "testsuite/" + testsuite

    date = str(datetime.datetime.now())
    date = date[8] + date[9] + '.' + date[5] + date[6]
    default_title = ' ' + ticket + ': ' + release.upper() + ' ' + date
    suites = get_suites(release, alice_suite_url, testpalm_header)

    if not suites:
        raise Exception('Can not get suites by ' + release + ' release tag')

    list_runs_for_assessors = []
    for item in suites:
        title = item["title"][:item["title"].find(']') + 1] + default_title  # сорян, некрасивый костыль
        tags = item["tags"]
        suite_id = item["id"]
        run = create_run(suite_id, title, tags, ticket, alice_run_suite_url, st_header, testpalm_header)
        if not run:
            LOGGER.error('Can not create run ' + title)
        else:
            run_id = run['id']
            line_for_list_runs = '{} https://testpalm.yandex-team.ru/{}/testrun/{}'.format(
                item["title"][:item["title"].find(']') + 1],
                testsuite,
                run_id,
            )
            list_runs_for_assessors.append(line_for_list_runs)

    return create_assessors_ticket(release, ticket, list_runs_for_assessors, st_header, uniproxy_url)


def create_run(suite_id, title, tags, ticket, alice_run_test_suite, st_header, testpalm_header):
    parentIssue = {
        "id": ticket,
        "trackerId": "Startrek",
    }
    params = {
        "include": "id,title",
    }
    payload = {
        "tags": tags,
        "title": title,
        "testSuite": {
            "id": suite_id,
        },
        "parentIssue": parentIssue,
    }

    response = requests.post(alice_run_test_suite, json=payload, params=params, headers=testpalm_header)
    if response.status_code != 200:
        LOGGER.error("Unable to create testpalm run, code: {code}, body: {body}".format(code=response.status_code,
                                                                                        body=response.text))
        return None

    run = next(iter(response.json()), {})
    # run_id = run['id']
    return run


def generate_headers(token=None):
    return {} if token is None else {
        'Authorization': 'OAuth {}'.format(token),
        'Content-Type': 'application/json',
    }


def get_suites(release, alice_suite_url, testpalm_header):
    params = {"include": "id,title,tags",
              "expression": '''{{"type": "CONTAIN", "key": "tags", "value": "{tag}" }}'''.format(tag=release)}
    resp = requests.get(alice_suite_url, headers=testpalm_header, params=params)
    return None if not resp.text else resp.json()


# This can be (should be) moved to startrek client api instead of requests
def create_assessors_ticket(release, ticket, runs, st_header, uniproxy_url):
    runs_in_ticket = ''
    for el in runs:
        runs_in_ticket += el + '\n'
    description = u"Релиз " + release + '\n\n'
    description += u"Релизный тикет " + ticket + '\n\n'
    description += u'Конфиг:\n"uniProxyUrl": "%%{}%%"\n'.format(uniproxy_url)
    description += u'"VINS" "http://vins.alice.yandex.net/speechkit/app/pa/"\n'
    description += u'"BASS": "http://localhost:86/"\n\n'

    # TODO: Колонки? (see https://st.yandex-team.ru/ALICEASSESSORS-2950 as example)
    description += u"Список ранов:\n\n" + '<[' + runs_in_ticket + ']>'
    description += '\n\n' + u"Инструкция по тестированию https://wiki.yandex-team.ru/alicetesting/assessors-and-alice/"
    data = {
        "queue": ASSESSORS_QUEUE,
        "type": "task",
        "summary": u"Тестирование релиза " + release + " " + ticket,
        "description": description,
        "parent": ticket,
    }
    raw_data = json.dumps(data, ensure_ascii=False, separators=(",", ": "))
    raw_data = raw_data.encode('utf-8')
    resp = requests.post(ST_API_URL, data=raw_data, headers=st_header)
    LOGGER.info("ST API RESPONSE {}".format(resp.json()))
    if not resp.json():
        raise Exception('bad response from ST API')

    return resp.json().get('key')
