import os
import random
import string
import time
import json
import sys
import shutil
import pytest
import logging

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.extlog.async_file_logger import AsyncFileLogger

letters_and_digits = string.ascii_uppercase + string.digits


def gen_random_string(n):
    return ''.join(random.choice(letters_and_digits) for _ in range(n))


test_directory = './test_data'
log_file = 'log.out'
processes_count = 10
messages_per_process_count = 50000
min_message_size = 100
max_message_size = 50 * 1024
random_swaps_count = 5
test_data = bytearray(gen_random_string(max_message_size), 'ascii')


def log_out(m):
    print(m)
    sys.stdout.flush()


def random_swap(data):
    i = random.randint(0, len(data) - 1)
    j = random.randint(0, len(data) - 1)
    v = data[i]
    data[i] = data[j]
    data[j] = v


def select_random_string(size):
    start = random.randint(0, len(test_data) - size)
    result = test_data[start:start + size]
    for i in range(0, random_swaps_count):
        random_swap(result)
    return result.decode('ascii')


def acceptable(stats):
    return stats.messages_count >= messages_per_process_count and \
        stats.errors_count == 0 and \
        stats.dropped_messages_count == 0


def run_worker(async_file_logger):
    with open('{0}/{1}'.format(test_directory, os.getpid()), 'a') as f:
        bytes_written = 0
        for x in range(0, messages_per_process_count):
            message_length = random.randint(min_message_size, max_message_size)
            m = select_random_string(message_length)
            Logger.get().warning(m)
            f.write(m)
            f.write('\n')
            bytes_written += message_length
            if (x + 1) % 1000 == 0:
                log_out('worker [{0}], messages [{1}, {2}%], bytes [{3}]'.format(
                        os.getpid(), x + 1, float(x + 1) / messages_per_process_count * 100, bytes_written))
    log_out('worker [{0}] finish write messages'.format(os.getpid()))
    async_file_logger.join()
    stats = async_file_logger.get_stats()
    log_out('worker [{0}] done: {1}'.format(os.getpid(), json.dumps(stats)))
    if not acceptable(stats):
        raise ValueError('worker [{0}] bad stats'.format(os.getpid()))


def check_test_data():
    counts_by_hash = {}
    for n in os.listdir(test_directory):
        log_out('checking [{0}]'.format(n))
        with open('{0}/{1}'.format(test_directory, n)) as f:
            if n == log_file:
                for l in f:
                    message_marker_index = l.index(': ')
                    m = l[message_marker_index + 2:len(l) - 2]
                    key = hash(m)
                    counts_by_hash[key] = counts_by_hash.get(key, 0) + 1
            else:
                for l in f:
                    m = l[:len(l) - 1]
                    key = hash(m)
                    counts_by_hash[key] = counts_by_hash.get(key, 0) - 1
        log_out('checking [{0}] done'.format(n))
    for k, v in counts_by_hash.items():
        if v != 0:
            raise ValueError('consistency check failed, key [{0}], value [{1}]'.format(k, v))


def run_workers(async_file_logger):
    start_time = time.time()
    worker_pids = []
    for i in range(0, processes_count):
        pid = os.fork()
        if pid == 0:
            run_worker(async_file_logger)
            os._exit(0)
        worker_pids.append(pid)
    log_out('workers started')
    for pid in worker_pids:
        _, status = os.waitpid(pid, 0)
        assert os.WIFEXITED(status)
        assert os.WEXITSTATUS(status) == 0
    finish_time = time.time()
    log_out('took [{0}] secs'.format(finish_time - start_time))


@pytest.fixture(autouse=True)
def handle_test_directory():
    if os.path.exists(test_directory):
        shutil.rmtree(test_directory)
    os.mkdir(test_directory)
    yield
    shutil.rmtree(test_directory)


# хочет много диска в pwd (50000 * 25 * 1024 * 10 * 2 = 25Gb), на tc столько нет, можно руками запускать
def _test_integration():
    async_file_logger = AsyncFileLogger({
        'file_name': '{0}/{1}'.format(test_directory, log_file),
        'block_on_full_queue': True
    })
    Logger.init('logging_integration_test', False, async_file_logger=async_file_logger)
    run_workers(async_file_logger)
    check_test_data()


def test_loglevel():
    file_name = '{0}/{1}'.format(test_directory, log_file)
    async_file_logger = AsyncFileLogger({
        'file_name': file_name,
        'block_on_full_queue': True
    })
    logger = logging.getLogger('test-logging')
    logger.setLevel(logging.INFO)
    logger.addHandler(async_file_logger.create_handler())
    logger.log(logging.DEBUG, 'this text must not be logged')
    logger.log(logging.WARNING, 'this text must be logged')
    async_file_logger.join()
    with open(file_name, 'r') as file:
        content = file.read()
    assert 'this text must not be logged' not in content
    assert 'this text must be logged' in content


def test_reopen_if_inode_changes():
    file_name = '{0}/{1}'.format(test_directory, log_file)
    async_file_logger = AsyncFileLogger({
        'file_name': file_name,
        'block_on_full_queue': True
    })
    logger = logging.getLogger('test-logging')
    logger.setLevel(logging.INFO)
    logger.addHandler(async_file_logger.create_handler())

    logger.log(logging.WARNING, 'this text must go to the first file')
    async_file_logger.join()

    bak_file_name = '{0}/{1}'.format(test_directory, log_file + '.bak')
    shutil.move(file_name, bak_file_name)

    logger.log(logging.WARNING, 'this text must go to the second file')
    async_file_logger.join()

    with open(bak_file_name, 'r') as file:
        bak_file_content = file.read()
        assert 'this text must go to the first file' in bak_file_content
    with open(file_name, 'r') as file:
        file_content = file.read()
        assert 'this text must go to the second file' in file_content
