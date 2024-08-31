import html
import json
import logging
import os
import os.path
import requests
import shutil
import socket
import subprocess
import sys

from urlparse import urljoin


logger = logging.getLogger(__name__)


class Config(object):
    def __init__(self, project, session_id,  workdir, binary_path, skip_headers=[], skip_cgis=[], stub_force_update=False, stub_fetch_if_not_exists=False):
        self.project = project
        self.session_id = session_id
        self.workdir = os.path.realpath(workdir)
        self.binary_path = binary_path
        self.skipped_headers = skip_headers
        self.skipped_cgis = skip_cgis
        self.stub_force_update = stub_force_update
        self.stub_fetch_if_not_exists = stub_fetch_if_not_exists

        self._s3_credentials = None

    def set_s3_credentials(self, id, secret):
        self._s3_credentials = {'KeyId': id, 'KeySecret': secret}

    def s3_credentials(self):
        return self._s3_credentials

    def threads(self):
        return 10  # FIXME (petrk) use params


class JokerSyncRequest:
    class Exception(Exception):
        pass

    def __init__(self, joker):
        self.joker = joker
        self.body = None

    def add_stub(self, data):
        if self.body is not None:
            self.body += '\n'
        else:
            self.body = ''

        self.body += 'sync\t{}'.format(data)

    def check_run(self):
        if self.body is None:
            return

        response = requests.post(urljoin(self.joker.scheme, 'sync_versioned'), params={'session_id': self.joker.session_id}, data=self.body)
        response.raise_for_status()
        resp_json = response.json()

        invalid_actions = []
        for action in resp_json['actions']:
            if 'success' not in action:
                invalid_actions.append(action)

        if invalid_actions:
            raise JokerSyncRequest.Exception(invalid_actions)


class JokerMocker:
    def __init__(self, config, port, log_dir, run_all, save_stubs='saved_stubs', url_requester=None):
        self.config = config
        self.log_dir = log_dir
        self.run_all = run_all
        if not os.path.exists(config.workdir):
            os.makedirs(config.workdir)

        self.port = port
        json_config = {
            'Backend': {
                'Type': 'sf',
                'Params': {
                    'Dir': os.path.realpath(save_stubs)
                }
            },
            'Server': {
                'HttpPort': self.port,
                'HttpThreads': config.threads(),
            },
            'WorkDir': config.workdir,
            'StubIdGenerator': {
                'SkipHeader': config.skipped_headers,
                'SkipCGI': config.skipped_cgis,
            }
        }

        logger.debug('joker config: %s', json_config)

        self.config_filename = os.path.join(config.workdir, 'config.json')
        with open(self.config_filename, 'w') as f:
            f.write(json.dumps(json_config, indent=1))

        with open(os.path.join(log_dir, 'joker'), 'w+') as file_out:
            self._io = subprocess.Popen([self.config.binary_path, 'server', self.config_filename], stdout=file_out, stderr=file_out)

        self.host_port = '{}:{}'.format(socket.gethostname(), self.port)
        self.scheme = 'http://{}'.format(self.host_port)

        if url_requester:
            # Wait until joker is started
            resp = url_requester('{}/admin?action=ping'.format(self.scheme))
            assert resp == 'pong'

        self.proxy_header = '{}:{}'.format(socket.gethostname(), self.port)

        # FIXME add posting urls
        session_params = {
            'id': config.session_id,
            'fetch_if_not_exists': config.stub_fetch_if_not_exists,
            'force_update': config.stub_force_update,
        }
        session = requests.post(urljoin(self.scheme, 'session'), params=session_params).json()
        if 'session_id' not in session:
            raise Exception('unable to obtain session from joker')

        self.session_id = html.escape(session['session_id'])
        # FIXME add check for errors

    def url(self, path):
        return urljoin(self.scheme, path)

    def stop(self):
        # shutdown joker gracefuly
        resp = requests.get(self.url('/admin'), params=[('action', 'shutdown')])
        logger.debug('joker stop response: %s', resp.content)
        self._io.wait()

    def create_cgi(self, req_id):
        return 'prj={}&sess={}&test={}'.format(html.escape(self.config.project), html.escape(self.session_id), html.escape(req_id))

    def insert_headers(self, headers, req_id):
        headers['x-yandex-via-proxy'] = self.proxy_header
        headers['x-yandex-proxy-header-x-yandex-joker'] = self.create_cgi(req_id)

        if self.run_all:
            headers['x-yandex-proxy-header-x-yandex-proxy-header-x-yandex-via-proxy'] = self.proxy_header
            headers['x-yandex-proxy-header-x-yandex-proxy-header-x-yandex-proxy-header-x-yandex-joker'] = self.create_cgi(req_id)
            headers['x-yandex-via-proxy-skip'] = 'Vins'

    def info(self):
        cmd = [self.config.binary_path, 'session', '--id', self.session_id, 'info', self.config_filename]
        with open(os.path.join(self.log_dir, 'joker_info'), 'w+') as file_out:
            run_io = subprocess.Popen(cmd, stdout=file_out, stderr=file_out, stdin=None)
        run_io.wait(timeout=20)

    def push(self, stubs_filename, rewrite=True):
        cmd = [self.config.binary_path, 'session', '--id', self.session_id, 'push', self.config_filename]
        logger.debug('Run joker: {}'.format(' '.join(cmd)))
        push_io = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=sys.stderr, stdin=None)
        while True:
            line = push_io.stdout.readline().decode('utf-8')
            if line != '':
                if line.startswith(' '):
                    with open(stubs_filename, 'w' if rewrite else 'a') as f:
                        f.write('1\t{}\n'.format(line.lstrip().rstrip()))
            else:
                break
        push_io.wait()

        if push_io.returncode:
            raise Exception("Something wrong with push command (rcode %d)", push_io.returncode)

        shutil.rmtree(self.config.workdir)
