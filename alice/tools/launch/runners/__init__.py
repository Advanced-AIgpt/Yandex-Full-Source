#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import logging
import os
import signal
import shutil
import subprocess
import time

from urllib.parse import urljoin


class Server:
    def __init__(self, name, bin_path, log_dir, config_path, port, *args):
        self._name = name
        self._bin_path = bin_path
        self._log_dir = log_dir
        self._config_path = config_path
        self._port = port
        self._args = args
        self._server = None

    def _start(self):
        raise NotImplementedError()

    def start(self):
        logging.info('Starting {}...'.format(self._name))
        self._start()
        logging.info('{} started'.format(self._name))

    def get_address(self):
        return 'localhost:{}'.format(self._port)

    def get_url(self):
        return 'http://' + self.get_address() + '/'

    def stop(self, server=None):
        if server is None:
            server = self._server
        if server is not None:
            try:
                logging.info('Stopping {}, pid = {}'.format(self._name, server.pid))
                server.send_signal(signal.SIGINT)
                server.wait(10)
                server = None
            except Exception:
                logging.info('Killing {}, pid = {}'.format(self._name, server.pid))
                server.kill()
                server = None

    def status(self):
        raise NotImplementedError()

    @staticmethod
    def _curl(name, check_status_cmd, successful_answer):
        try:
            logging.info('Requesting {}'.format(name))
            ret = subprocess.check_output([check_status_cmd], shell=True)
            if ret == successful_answer:
                logging.info('Success answer for {}'.format(name))
                return True
            else:
                logging.info('Wrong answer from {}, got {}, expected {}'.format(name, ret, successful_answer))
                return False
        except Exception:
            logging.info('No answer from {}'.format(name))
            return False


class BassServer(Server):
    def __init__(self, bin_path, log_dir, config_path, port, package_path, *args):
        super(BassServer, self).__init__('bass', bin_path, log_dir, config_path, port, *args)
        self._package_path = package_path

    def start(self):
        args = self._args
        args += ('-V', 'EventLogFile=' + self._log_dir + '/current-bass-rtlog')
        args += ('-V', 'ENV_GEOBASE_PATH={}'.format(self._package_path + '/geodata6.bin'))
        args += ('--logdir', self._log_dir)
        logging.info('Starting {}'.format(self._name))
        self._server = subprocess.Popen([self._bin_path, self._config_path, '--port', str(self._port), *args])

    @staticmethod
    def ping(url):
        check_status_cmd = 'curl {}ping'.format(url)
        return Server._curl('bass', check_status_cmd, b'pong')

    @staticmethod
    def reload_logs(url):
        check_status_cmd = 'curl {}reopenlogs'.format(url)
        return Server._curl('bass', check_status_cmd, b'OK')

    def status(self):
        return BassServer.ping(self.get_url())


class VinsServer(Server):
    def __init__(self, bin_path, log_dir, config_path, port, package_path, bass_url=None, redis_port=None, *args):
        super(VinsServer, self).__init__('vins', bin_path, log_dir, config_path, port, *args)
        self._package_path = package_path
        self._bass_url = bass_url
        self._redis_port = redis_port

    def start(self):
        if self._bass_url:
            os.environ['VINS_DEV_BASS_API_URL'] = self._bass_url
        if self._redis_port:
            os.environ['VINS_REDIS_PORT'] = str(self._redis_port)
        os.environ['VINS_RTLOG_FILE'] = os.path.join(self._log_dir, 'current-vins-rtlog')
        os.environ['VINS_LOG_FILE'] = os.path.join(self._log_dir, 'vins.push_client.out')

        log_file = os.path.join(self._log_dir, '{}.txt'.format(self._name))
        args = self._args
        work_dir = os.getcwd()
        os.chdir(self._package_path)
        logging.info('Starting {} (It\'s OK to wait for some minutes)'.format(self._name))
        with open(log_file, "w") as f_err:
            self._server = subprocess.Popen([self._bin_path, '-p', str(self._port), '--conf-dir', self._config_path, *args], stderr=f_err, stdout=f_err)
        os.chdir(work_dir)

    @staticmethod
    def ping(url):
        check_status_cmd = 'curl {}ping'.format(url)
        return Server._curl('vins', check_status_cmd, b'Ok')

    @staticmethod
    def reload_logs(url):
        return True

    def status(self):
        return VinsServer.ping(self.get_url())


class MegamindServer(Server):
    def __init__(self, bin_path, log_dir, config_path, port, package_path, vins_url=None, bass_url=None, *args):
        super(MegamindServer, self).__init__('megamind', bin_path, log_dir, config_path, port, *args)
        self._package_path = package_path
        self._vins_url = vins_url
        self._bass_url = bass_url

    def start(self):
        log_file = os.path.join(self._log_dir, '{}.txt'.format(self._name))
        args = self._args
        if self._vins_url:
            try_num = 0
            while not VinsServer.ping(self._vins_url):
                try_num += 1
                if try_num >= 100:
                    logging.info('Vins is unavailable. Starting megamind without Vins')
                    break
                time.sleep(10)
            else:
                args += ('--service-sources-vins-url', self._vins_url)
        if self._bass_url:
            args += ('--scenarios-sources-bass-url', self._bass_url)
            args += ('--scenarios-sources-bass-apply-url', urljoin(self._bass_url, '/megamind/apply'))
            args += ('--scenarios-sources-bass-run-url', urljoin(self._bass_url, '/megamind/run'))

        args += ('--rtlog-filename', self._log_dir + '/current-megamind-rtlog')
        args += ('--vins-like-log-file', self._log_dir + '/current-megamind-vins-like-log')
        args += ('--geobase-path', self._package_path + '/geodata6.bin')
        logging.info('Starting {}'.format(self._name))
        with open(log_file, "w") as f_err:
            self._server = subprocess.Popen([self._bin_path, '-c', self._config_path, '-p', str(self._port), *args], stderr=f_err, stdout=f_err)

    @staticmethod
    def ping(url):
        check_status_cmd = 'curl {}ping'.format(url)
        return Server._curl('megamind', check_status_cmd, b'pong')

    @staticmethod
    def reload_logs(url):
        check_status_cmd = 'curl {}reload-logs'.format(url)
        return Server._curl('megamind', check_status_cmd, b'done')

    def status(self):
        return MegamindServer.ping(self.get_url())


class RedisServer(Server):
    def __init__(self, bin_path, log_dir, config_path, port, *args):
        super(RedisServer, self).__init__('redis', bin_path, log_dir, config_path, port, *args)

    def start(self):
        base_config_path = self._config_path
        config_path = os.path.join(os.path.dirname(base_config_path), 'redis.conf')
        shutil.copy(base_config_path, config_path)
        f = open(config_path, 'a')
        f.write('port {}'.format(self._port))
        f.close()
        log_file = os.path.join(self._log_dir, 'redis.txt')
        args = self._args
        logging.info('Starting {}'.format(self._name))
        with open(log_file, "w") as f_err:
            self._server = subprocess.Popen([self._bin_path, config_path, *args], stderr=f_err, stdout=f_err)

    def status(self):
        return True
