import argparse
import random
import hashlib

from alice.cachalot.api.protos.cachalot_pb2 import TRequest


def gen_random_key(is_curl=False, maxvalue=1000):
    return 'Cache.Key.%5.5d' % (random.randint(0, maxvalue) if not is_curl else 0)


def gen_serial_key(is_curl, index):
    return 'Cache.Key.%5.5d' % (index,)


def gen_set_request(key, min_request_size, max_request_size, redis):
    value = gen_random_data(min_request_size, max_request_size)
    req = TRequest()
    if redis:
        req.RequestOptions.ForceRedisStorage = True
    req.SetReq.Key = hashlib.md5(key.lower().encode('utf8')).hexdigest()
    req.SetReq.Data = value
    return req.SerializeToString()


def gen_get_request(key, redis):
    req = TRequest()
    if redis:
        req.RequestOptions.ForceRedisStorage = True
    req.GetReq.Key = hashlib.md5(key.lower().encode('utf8')).hexdigest()
    return req.SerializeToString()


def gen_random_data(minsize, maxsize):
    return 'x' * random.randint(minsize, maxsize)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--count', type=int, help='requests count', default=1000)
    parser.add_argument('-s', '--set-requests-perc', type=float, help='perc of set requests', default=0.2)
    parser.add_argument('-o', '--output', type=str, help='file to store ammos', required=True)
    parser.add_argument('-n', '--min-request-size', type=int, help='minimum request size', default=7000)
    parser.add_argument('-x', '--max-request-size', type=int, help='minimum request size', default=30000)
    parser.add_argument('-r', '--redis', action='store_true', help='generate request for redis backend')
    parser.add_argument('--randomize-keys', type=int, default=0, help='genrate random keys in range')
    parser.add_argument('--curl', action='store_true', help='generate curl request body')
    args = parser.parse_args()

    with open(args.output, 'wb') as ammo:
        ammo.write(b'[Content-Type: application/protobuf]\n')

        for i in range(0, args.count if not args.curl else 1):
            p = random.random()

            if args.randomize_keys > 0:
                key = gen_random_key(args.curl, args.randomize_keys)
            else:
                key = gen_serial_key(args.curl, i)
            if p < args.set_requests_perc:
                req = gen_set_request(key, args.min_request_size, args.max_request_size, args.redis)
            else:
                req = gen_get_request(key, args.redis)

            if not args.curl:
                ammo.write(b"[X-Cachalot-Key: %s]\n" % (key,))
                ammo.write(b"%d /cache/proto\n%s\n" % (len(req), req))
            else:
                ammo.write(req)
