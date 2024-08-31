from alice.library.python.utils.arcadia import arcadia_root

import argparse
import logging
import os.path
import subprocess
import sys
import shutil

from alice.joker.library.python.run_info import RunInfo
from alice.megamind.tests.library.settings import Settings
from alice.megamind.tests.library.ya_tool import ya_tool


CURRENT_DIR = os.path.dirname(sys.argv[0])
TESTS_DIR = os.path.realpath(os.path.join(CURRENT_DIR, '..'))

logger = logging.getLogger(__name__)


class Context:
    def __init__(self, args):
        self.joker_dir = os.path.join(TESTS_DIR, '.joker.temp')
        # it is needed to save/load info about joker_bin and its config locations
        self.joker_runinfo_file = os.path.join(self.joker_dir, 'run_info.{}.txt'.format(args.session))
        self.settings = Settings()
        self.ya_make = YaMake(args, self)

    def update_joker(self):
        # TODO (petrk) path to joker must be somewhere else
        cmd = self.ya_make.ya_make_cmd + [os.path.join(arcadia_root(), 'alice/joker/bin')]

        result_io = subprocess.Popen(cmd, stdout=sys.stdout, stderr=sys.stderr)
        result_io.wait()
        if result_io.returncode:
            logger.error('Unable to make joker moker:\n%s', result_io.communicate()[1].readall())
            sys.exit(result_io.returncode)


class YaMake:
    def __init__(self, args, ctx):
        self.ctx = ctx
        # FIXME (petrk) create a separate function
        self.ya_make_cmd = [ya_tool(), 'make'] + args.ya_make_opts
        self.test_cmd = self.ya_make_cmd + ['-ttt', '--keep-temps', '--test-debug', '--test-stderr']
        self.test_cmd.append(TESTS_DIR)

    def run_tests(self, session_id, stub_fetch_if_not_exists=False, stub_force_update=False, stub_force_update_for_non_request=False):
        newenv = os.environ.copy()
        self.ctx.settings.set_runinfo_file(self.ctx.joker_runinfo_file, newenv)
        self.ctx.settings.set_workdir(self.ctx.joker_dir, newenv)
        self.ctx.settings.set_session_id(session_id, newenv)
        self.ctx.settings.set_arcadia_root(arcadia_root(), newenv)
        if stub_fetch_if_not_exists:
            self.ctx.settings.enable_fetch_if_not_exists(newenv)
        if stub_force_update:
            self.ctx.settings.enable_force_update(newenv)

        # FIXME (petrk) added handling SIGINT and propogate it to ```ya make -t```.
        # Right now this script is canceled but ```ya make -t``` is not!
        run = subprocess.Popen(self.test_cmd, stdout=sys.stdout, stderr=sys.stderr, stdin=None, env=newenv)
        run.wait()


def run_tests(ctx, args):
    if args.from_scratch and os.path.exists(ctx.joker_dir):
        logger.debug('Remove old joker working dir: %s', ctx.joker_dir)
        shutil.rmtree(ctx.joker_dir)

    yaMake = YaMake(args, ctx)
    yaMake.run_tests(
        args.session,
        stub_fetch_if_not_exists=args.stub_fetch_if_not_exists,
        stub_force_update=args.stub_force_update,
        stub_force_update_for_non_request=True,
    )


def session_info(ctx, args):
    try:
        ctx.update_joker()

        cmd = RunInfo.load_from_file(ctx.joker_runinfo_file).joker_cmd('info')
        logger.debug('Run joker: {}'.format(' '.join(cmd)))
        run_io = subprocess.Popen(cmd, stdout=sys.stdout, stderr=sys.stderr, stdin=None)
        run_io.wait(timeout=20)
        sys.exit(run_io.returncode)
    except FileNotFoundError:
        raise Exception('You have to (re)run `tests` before use `info`')


def session_push(ctx, args):
    try:
        ctx.update_joker()

        push_cmd = RunInfo.load_from_file(ctx.joker_runinfo_file).joker_cmd('push')
        logger.debug('Run joker: {}'.format(' '.join(push_cmd)))
        push_io = subprocess.Popen(push_cmd, stdout=subprocess.PIPE, stderr=sys.stderr, stdin=None)

        test_meta = None
        while True:
            line = push_io.stdout.readline().decode('utf-8')
            if line != '':
                if line.startswith(' '):
                    if test_meta is None:
                        raise Exception('unexpected joker output')

                    test_id = test_meta.pop('test_id', None)
                    with open(test_meta['file'], 'w' if test_id else 'a') as f:
                        f.write('1\t{}\n'.format(line.lstrip().rstrip()))
                else:
                    # TODO (petrk) use a separate class from joker to parse this answer
                    test_meta = {
                        'file': os.path.join(TESTS_DIR, 'stubs', line.rstrip().split('/')[1] + '.txt'),
                        'test_id': line.rstrip()
                    }
            else:
                break

        push_io.wait()
        if push_io.returncode:
            raise Exception('Something wrong with push command (rcode %d)', push_io.returncode)

        # TODO (petrk) Add cleaning.
        # clear_cmd = RunInfo.load_from_file(ctx.joker_runinfo_file).joker_cmd('clear')
        # subprocess.run(clear_cmd, stdout=subprocess.PIPE, stderr=sys.stderr, stdin=None).check_returncode()

    except FileNotFoundError:
        raise Exception('You have to (re)run `tests` before use `push`')


def main():
    logger.setLevel(logging.INFO)
    handler = logging.StreamHandler(sys.stderr)
    logger.addHandler(handler)

    args = parse_args()
    logger.debug(args)
    args.func(Context(args), args)


def parse_args():
    epilog = """
Usual cases:
 Check tests:
  ./tests.py tests  # run tests and (-n) collect and save data from new sources

 New tests
  ./tests.py run -n -F 'some::new::test'  # run tests (specified in -F ..) and gather (-n) unsaved requests
  ./tests.py info  # show the information about last run `tests` command
  ./tests.py push  # upload all the modified stub data to persistant storage and modify stubmeta file which you should commit
"""
    parser = argparse.ArgumentParser(description='a runner for tests and checker for new/changed responses.', epilog=epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
    subparsers = parser.add_subparsers(help='Actions (default `tests`)', dest='action')

    parser_run_tests = subparsers.add_parser('tests', help='Run tests')
    # FIXME (petrk) add session name checking
    parser_run_tests.add_argument('-v', '--verbose', action='count', help='verbosity level')
    parser_run_tests.add_argument('-s', '--session', default='default', type=str, metavar='SESSION-NAME', help='create a separate session with given name')
    parser_run_tests.add_argument('-n', '--stub-fetch-if-not-exists', action='store_true', help='Make a remote request if stub does not exist')
    parser_run_tests.add_argument('-f', '--stub-force-update', action='store_true', help='Force remote refetch even if it has stub')
    parser_run_tests.add_argument('-z', '--from-scratch', action='store_true', help='Run from scratch, remove all previous working data')
    parser_run_tests.add_argument('ya_make_opts', nargs='*', help='Ya make options pass to tests')
    parser_run_tests.set_defaults(func=run_tests)

    parser_info = subparsers.add_parser('info', help='Show results which are made by `tests` command.')
    parser_info.add_argument('-v', '--verbose', action='count', help='verbosity level')
    parser_info.add_argument('-s', '--session', default='default', type=str, metavar='SESSION-NAME', help='create a separate session with given name')
    parser_info.add_argument('-t', '--tags', metavar='TAGS', nargs='*', choices=['modified', 'deleted'], help='verbosity tags %(choices)s')
    parser_info.add_argument('ya_make_opts', nargs='*', help='Ya make options')
    parser_info.set_defaults(func=session_info)

    parser_push = subparsers.add_parser('push', help='Push results (which are made by `tests` command) to the persistent storage.')
    parser_push.add_argument('-v', '--verbose', action='count', help='verbosity level')
    parser_push.add_argument('-s', '--session', default='default', type=str, metavar='SESSION-NAME', help='create a separate session with given name')
    parser_push.add_argument('ya_make_opts', nargs='*', help='Ya make options')
    parser_push.set_defaults(func=session_push)

    return parser.parse_args()
