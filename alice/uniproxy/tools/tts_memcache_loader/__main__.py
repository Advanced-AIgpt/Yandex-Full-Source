import argparse
import json
import sys
import subprocess
import os
import tornado
from tornado import gen

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from alice.uniproxy.library.backends_tts.cache import CacheStorageClient, is_cachable
from alice.uniproxy.library.backends_tts.ttsutils import cache_key, tts_response_to_cache, split_text_by_speaker_tags


success_counter = 0
all_counter = 0
all_sum = 0


def setup(args):
    GlobalCounter.init()
    UniproxyCounter.init()


def params_from_spec(spec, args):
    if not spec.get('text'):
        return False, None

    params = {
        'voice': 'shitova.gpu'
    }

    if args.voice is not None:
        params['voice'] = args.voice

    queue = split_text_by_speaker_tags(spec['text'])
    if len(queue) != 1:
        return False, None
    params['text'] = queue.pop(0).get('text')
    return True, params


def data_from_file(path):
    in_file = open(path, 'rb')
    data = in_file.read()
    in_file.close()
    process = subprocess.Popen(['file', path], stdout=subprocess.PIPE)
    res = process.communicate()[0].decode('utf-8').lower()
    process.wait()
    if 'opus' not in res:
        return False, None
    return True, data


def inc_counters(ok=False):
    global success_counter, all_counter, all_sum
    if ok:
        success_counter += 1
    all_counter += 1
    if all_counter == all_sum:
        print('Done {}/{}'.format(success_counter, all_counter))
        tornado.ioloop.IOLoop.current().stop()


@gen.coroutine
def process_record(spec, args):
    try:
        opus_path = '{}/{}'.format(args.data, spec['file'])
        status, params = params_from_spec(spec, args)
        if not status:
            raise Exception('uncorrect params in spec: {}'.format(spec))
        status, data = data_from_file(opus_path)
        if not status:
            raise Exception('uncorrect opus file: {}'.format(opus_path))

        res = yield post(params, data, args)
    except Exception as e:
        print('Exception: {}'.format(e))
        res = False
    finally:
        inc_counters(res)


@gen.coroutine
def post(params, data, args):
    if is_cachable(params['text']):
        res = yield CacheStorageClient().store(
            cache_key(params),
            tts_response_to_cache(data, 0),
            ttl=args.ttl
        )
        if not res:
            print('can not post')
        return res
    print('is not cachable')
    return False


@gen.coroutine
def iterate(args):
    spec_path = args.spec
    try:
        with open(spec_path, 'r') as f:
            specs = json.load(f)
        assert isinstance(specs, list)
    except Exception as e:
        print('Error while reading spec: {}'.format(e))
        sys.exit(0)

    global all_sum
    all_sum = len(specs)
    for spec in specs:
        yield process_record(spec, args)


def main():

    if 'UNIPROXY_CUSTOM_ENVIRONMENT_TYPE' not in os.environ:
        print('Please, set UNIPROXY_CUSTOM_ENVIRONMENT_TYPE. If you want prod, use "rtc_production"')
        sys.exit(0)

    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--data',
        type=str,
        help='path to data',
        required=True
    )
    parser.add_argument(
        '--spec',
        type=str,
        help='path to json spec',
        required=True
    )
    parser.add_argument(
        '--ttl',
        help='record ttl in memcache',
        default=None
    )
    parser.add_argument(
        '--voice',
        type=str,
        default=None,
        help='voice'
    )

    args = parser.parse_args()

    setup(args)
    tornado.ioloop.IOLoop.current().spawn_callback(iterate, args)
    tornado.ioloop.IOLoop.current().start()


if __name__ == '__main__':
    main()
