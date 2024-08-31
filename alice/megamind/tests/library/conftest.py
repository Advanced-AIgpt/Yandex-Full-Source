import alice.joker.library.python as joker_module
import alice.megamind.tests.library.megamind as megamind

from alice.megamind.tests.library.request import SpeechKit
from alice.megamind.tests.library.session import SessionManager
from library.python.vault_client import instances as YAV

import logging
import os.path
import pytest
import requests
import yatest.common

from retry import retry


logger = logging.getLogger(__name__)

# The environment variable from which OAuth token will be read to access s3-mds service.
S3_ENV_NAME = 'S3_STUB_ACCESS'

# It is a kind of a fake.
# Megamind has some sources which are requested in background regardless megamind requests.
# So this is a name for a fake test where these stubs are placed.
NON_REQUESTABLE_TEST_NAME = 'non_requestable_stubs'


@retry((ConnectionError, requests.exceptions.RequestException), tries=180, delay=0.5)
def wait_for_christmas(url, check_condition=None):
    """
    check_condition must raise an exception if wants to stop retrying
    """
    if check_condition:
        check_condition()
    return requests.get(url).text


class JokerConfig(joker_module.Config):
    def __init__(self, env_data, settings):
        super().__init__(
            project='megamind',
            s3_bucket_name='megamind-test-stubs',
            s3_host='s3.mds.yandex.net',
            arcadia_root=env_data['ARCADIA_ROOT'],
            workdir=settings.get_workdir('.joker.temp'),
            runinfo_file=settings.get_runinfo_file(),
            session_id=settings.get_session_id(),
            skip_headers=[
                'oauth',  # User token
                'x-rtlog-token',  # Token always changes
                'x-ya-service-ticket',  # Blackbox always uses current timestamp in requests
            ],
            skip_cgis=[
                'userip',  # For blackbox source
                'ts',
                'sign',
            ],
            stub_force_update=settings.is_force_update_enabled(),
            stub_fetch_if_not_exists=settings.is_fetch_if_not_exists_enabled(),
        )

        if env_data:
            creds = env_data[S3_ENV_NAME].split(sep=';', maxsplit=1)
            if len(creds) != 2 or len(creds[0]) == 0 or len(creds[1]) == 0:
                raise Exception('Invalid s3 credentials in "%s", must be "id;secret"', S3_ENV_NAME)
            self.set_s3_credentials(creds[0], creds[1])


class Environment:
    def __init__(self):
        self.session = SessionManager(yatest.common.source_path(os.path.join(yatest.common.context.project_path, 'stubs')))

        arcadia_root = self.session.settings.get_arcadia_root(yatest.common.build_path())

        params = dict()
        # If this env exists it means that this script is run under sandbox.
        if 'VAULT_CLIENT' in os.environ:
            token_file = os.environ['VAULT_CLIENT']
            with open(token_file) as f:
                token = f.read().strip()
                logger.debug("checksum: %s", token)
                params['authorization'] = 'OAuth ' + token

        yav = YAV.Production(**params)
        self.env_data = os.environ
        self.env_data.update(yav.get_version('sec-01cnbk6vvm6mfrhdyzamjhm4cm').get('value', {}))  # FIXME (petrk) add ability to override secretid
        self.env_data['ARCADIA_ROOT'] = arcadia_root

        # Run joker.
        self.joker = joker_module.JokerMocker(JokerConfig(self.env_data, self.session.settings), url_requester=wait_for_christmas)

        # Sync stubs not regarded to requests.
        self.session.sync_stubs(self.joker, NON_REQUESTABLE_TEST_NAME)

        # Megamind must be started right after sync is done.
        self.megamind = megamind.MegamindServer(NON_REQUESTABLE_TEST_NAME, self.joker, self.env_data, url_requester=wait_for_christmas)

    def tear_down(self):
        try:
            # FIXME add waiting timeout
            self.megamind.stop()
        except Exception as e:
            logger.error('Unable to stop megamind server %s', e)

        try:
            # FIXME add waiting timeout
            self.joker.stop()
        except Exception as e:
            logger.error('Unable to stop joker server %s', e)

        self.joker.write_runinfo()

        if yatest.common.context.test_stderr:
            pass  # TODO `joker_ctl` info must be there


@pytest.fixture(scope='session')
def env(request):
    env = Environment()

    def fin():
        env.tear_down()

    request.addfinalizer(fin)
    return env


@pytest.fixture(scope='function')
def speechkit(request, env):
    return SpeechKit(request.node.location, env.session, env.megamind, env.joker)
