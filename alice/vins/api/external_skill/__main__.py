import optparse

from alice.vins.api_helper import gunicorn_app
from vins_api.external_skill import gunicorn_conf
from vins_api.external_skill import api


def parse_args():
    parser = optparse.OptionParser()
    parser.add_option('-b', '--bind')
    parser.add_option('-w', '--workers')
    parser.add_option('--timeout')
    return parser.parse_args()


if __name__ == '__main__':
    cmd, _ = parse_args()

    opts = gunicorn_conf.__dict__  # XXX
    if cmd.bind:
        opts['bind'] = cmd.bind
    if cmd.workers:
        opts['workers'] = cmd.workers
    if cmd.timeout:
        opts['timeout'] = cmd.timeout

    gunicorn_app.GunicornApp(api.app, opts).run()
