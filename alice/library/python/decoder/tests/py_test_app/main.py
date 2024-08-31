
from alice.library.python.decoder import Decoder
import gc
import os


_PROC_STATUS = '/proc/{}/status'.format(os.getpid())
_SCALE = {
    'kB': 1024, 'mB': 1024*1024,
    'KB': 1024, 'MB': 1024*1024
}


def _VmB(VmKey):
    # get pseudo file  /proc/<pid>/status
    try:
        t = open(_PROC_STATUS)
        v = t.read()
        t.close()
    except:
        return 0  # non-Linux?
    # get VmKey line e.g. 'VmRSS:  9999  kB\n ...'
    i = v.index(VmKey)
    v = v[i:].split(None, 3)  # whitespace
    if len(v) < 3:
        return 0  # invalid format?
    # convert Vm value to bytes
    return int(v[1]) * _SCALE[v[2]]


def memory(since=0):
    """ Return memory usage in bytes """
    return _VmB('VmSize:') - since


def resident(since=0):
    """ Return resident memory usage in bytes """
    return _VmB('VmRSS:') - since


def decode_file(input_file, output_file, decoder=None, sample_rate=16000):
    if decoder is None:
        decoder = Decoder(sample_rate)

    with open(input_file, 'rb') as fi, open(output_file, 'wb') as fo:
        while True:
            data = fi.read(1024)
            if data:
                decoder.write(data)
            else:
                decoder.close()

            while True:
                data = decoder.read()
                if data is None:
                    break
                fo.write(data)

            if decoder.eof():
                break


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Audio file decoder")
    parser.add_argument("--input", required=True, help="Input (encoded) audio file")
    parser.add_argument("--output", required=True, help="Output (raw PCM) audio file")
    parser.add_argument("--sample-rate", type=int, required=False, default=16000, help="Output sample rate")
    parser.add_argument("--rounds", type=int, required=False, default=1, help="Decode rounds (to test memory leak)")

    args = parser.parse_args()

    print("Decode {} to {}; sample rate {}; rounds {}".format(args.input, args.output, args.sample_rate, args.rounds))

    warmup_rounds = 30
    start_rss = resident()

    for i in range(0, args.rounds):
        decode_file(input_file=args.input, output_file=args.output, sample_rate=args.sample_rate)
        rss = resident()
        if i % 100 == 0:
            print("#{} VmRSS={}".format(i, rss))
        if i == warmup_rounds:
            start_rss = rss

    gc.collect()

    if args.rounds > warmup_rounds:
        end_rss = resident()
        leak_per_run = float(end_rss - start_rss)/(args.rounds - warmup_rounds)
        print("leak_per_run={} (start rss={}, end rss={}, runs={})".format(leak_per_run, start_rss, end_rss, args.rounds))

    print("Done")
