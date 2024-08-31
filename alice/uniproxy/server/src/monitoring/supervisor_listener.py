#! /usr/bin/python
import sys
import os
import subprocess


def write_stdout(s):
    sys.stdout.write(s)
    sys.stdout.flush()


def write_stderr(s):
    sys.stderr.write(s)
    sys.stderr.flush()


def main(filename, env, logfile):
    environment = {
        'UNIPROXY_CUSTOM_ENVIRONMENT_TYPE': os.environ.get('UNIPROXY_CUSTOM_ENVIRONMENT_TYPE', ''),
        'QLOUD_ENVIRONMENT': os.environ.get('QLOUD_ENVIRONMENT', '')
    }
    environment.update({k: v for k, v in (a.split("=") for a in env.split(","))})

    while 1:
        try:
            write_stdout('READY\n')  # transition from ACKNOWLEDGED to READY
            line = sys.stdin.readline()  # read header line from stdin
            write_stderr(line)  # print it out to stderr
            headers = dict([x.split(':') for x in line.split()])
            data = sys.stdin.read(int(headers['len']))  # read the event payload

            with open(logfile, "w") as outfile:
                res = subprocess.call(
                    [filename],
                    env=environment,
                    stdout=outfile)
            write_stderr(data)
            write_stdout('RESULT 2\nOK')  # transition from READY to ACKNOWLEDGED
        except Exception:
            pass


if __name__ == '__main__':
    main(*sys.argv[1:])
