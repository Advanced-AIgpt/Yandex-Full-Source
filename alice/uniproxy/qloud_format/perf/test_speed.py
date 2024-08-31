#!/usr/bin/env python
import subprocess
import time


def run(cmd, records, repeat, stdout):
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=stdout)
    n = 0

    begin_ts = time.perf_counter()
    while n < repeat:
        n += 1
        for r in records:
            proc.stdin.write(r)
    end_ts = time.perf_counter()
    return end_ts - begin_ts


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--cmd", required=True, help="Command to run qloud_format program")
    parser.add_argument("--logs", required=True, help="File contains logs")
    parser.add_argument("--repeat", type=int, default=1, help="Repeat count")
    parser.add_argument("--print", action="store_true", help="Don't drop output")

    args = parser.parse_args()

    records = []
    with open(args.logs, "r") as f:
        for l in f.readlines():
            records.append(b"1000;2000;3000;" + l.encode("utf-8") + b"\n")
    print("{} log records are ready".format(len(records)))

    dur = run(args.cmd, records, args.repeat, None if args.print else subprocess.DEVNULL)
    print("It took {:.3f} sec".format(dur))


if __name__ == "__main__":
    main()
