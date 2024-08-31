import argparse
import subprocess
import pathlib
from jupytercloud.tools.lib import utils

PUBLIC_VM = "analytics-myt-01.taxi.dev.yandex.net"

COPY_TEMPLATE = [
    'ssh',
    '-A',
    '{old_vm_url}',
    "sudo rsync -e 'ssh -o StrictHostKeyChecking=no' -azxv /home/{login}/ {login}.user.jupyter.yandex-team.ru:{dst_dir}"
]


DU_TEMPLATE = [
    'ssh',
    '-A',
    '{old_vm_url}',
    'sudo du -d0 -BG -x /home/{login}'
]

SHUTDOWN_TEMPLATE = [
    'ssh', '{old_vm_url}', 'sudo shutdown 0'
]


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )
    parser.add_argument(
        '--input', type=argparse.FileType('r'), required=True
    )
    parser.add_argument('--dry-run', action='store_true')

    subparsers = parser.add_subparsers(dest='command')

    subparsers.add_parser('rsync').set_defaults(handler=do_rsync)
    subparsers.add_parser('shutdown').set_defaults(handler=do_shutdown)

    du = subparsers.add_parser('du')
    du.set_defaults(handler=do_du)
    du.add_argument('--max', default=90)
    du.add_argument('--min', default=10)

    return parser.parse_args()


def load_users(input, load_public_vm=True):
    result = {}

    for line in input:
        parts = line.strip().split('\t')
        if not parts:
            continue

        parts = [p.strip() for p in parts]

        login, have_jupyter, *vms = parts

        if have_jupyter == '1':
            have_jupyter = True
        elif have_jupyter in ('0', ''):
            have_jupyter = False
        else:
            raise ValueError(f'bad have jupyter column {have_jupyter}')

        vms = [vm for vm in vms if vm and (load_public_vm or vm != PUBLIC_VM)]
        if not vms:
            if load_public_vm:
                raise ValueError(f'empty vm list for {login}')
            else:
                continue

        result[login] = {
            'have_jupyter': have_jupyter,
            'vms': vms
        }

    return result


def generate_commands(users, cache_file, command_template):
    done = set()

    if cache_file is not None:
        cache_file = pathlib.Path(cache_file)
        if cache_file.exists():
            done = {line.strip() for line in cache_file.read_text().split('\n')}

    result = {}

    for login, info in users.items():
        vms = info['vms']

        dst_dir_template = f'/home/{login}'
        if info['have_jupyter'] or len(vms) > 1:
            dst_dir_template += '/{old_vm_url}/'

        for vm in vms:
            dst_dir = dst_dir_template.format(old_vm_url=vm)

            vars_ = {
                'old_vm_url': vm,
                'login': login,
                'dst_dir': dst_dir,
            }

            command = [c.format(**vars_) for c in command_template]

            if repr(command) not in done:
                result.setdefault(login, [])
                result[login].append(command)

        if not result.get(login):
            logger.info('skipping user %s, already done', login)

    return result


def execute(cmd):
    def log_subprocess_output(pipe):
        for line in iter(pipe.readline, b''):
            logger.debug('%r output: %r', cmd, line)

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    while process.poll() is None:
        log_subprocess_output(process.stdout)

    exitcode = process.wait()

    if exitcode:
        cmd = ' '.join(cmd)
        raise RuntimeError(f'{cmd} exited with non-zero exit code, {exitcode}')


def execute2(cmd):
    result = []

    def log_subprocess_output(pipe):
        for line in iter(pipe.readline, b''):
            logger.debug('%r output: %r', cmd, line)

    def buffer_output(pipe):
        for line in iter(pipe.readline, b''):
            if line := line.strip():
                result.append(line)

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    while process.poll() is None:
        log_subprocess_output(process.stderr)
        buffer_output(process.stdout)

    process.wait()

    return result


def do_copy(user_commands, dry_run, cache_file):
    for login, commands in user_commands.items():
        logger.info('starting copying user %s (%d commands)', login, len(commands))

        for command in commands:
            logger.info('for user %s executig command %r', login, ' '.join(command))

            if not dry_run:
                execute(command)

                with open(cache_file, 'a') as file_:
                    print(repr(command), file=file_)


def do_rsync(args):
    users = load_users(args.input)

    cache_file = args.input.name + '.cache'

    commands = generate_commands(users, cache_file, COPY_TEMPLATE)

    do_copy(commands, args.dry_run, cache_file)


def do_du(args):
    users = load_users(args.input)

    user_commands = generate_commands(users, None, DU_TEMPLATE)

    for login, commands in user_commands.items():
        total = 0
        error = False

        buf = []

        for command in commands:
            pretty_command = ' '.join(command)
            logger.debug('for user %s executig command %r', login, pretty_command)

            if not args.dry_run:
                result = execute2(command)

                if not result:
                    error = True
                    buf.append(f'{login}\tERROR\t{pretty_command}')
                else:
                    raw_size = result[0]
                    raw_size = raw_size.split()[0]
                    raw_size = raw_size.decode('utf-8')
                    size = int(raw_size.rstrip('G'))
                    total += size

                    buf.append(f'{login}\t{raw_size}\t{pretty_command}')

        if total >= args.min or error:
            logger.debug('user %s is bad', login)

            for line in buf:
                print(line)

            if total >= args.max:
                print(f'{login}\tMAX TOTAL VIOLATION\t')
            elif total >= args.min:
                print(f'{login}\tMIN TOTAL VIOLATION\t')
        else:
            logger.debug('user %s is normal', login)


def do_shutdown(args):
    users = load_users(args.input, load_public_vm=False)

    cache_file = args.input.name + '.shutdown-cache'
    user_commands = generate_commands(users, cache_file, SHUTDOWN_TEMPLATE)

    for login, commands in user_commands.items():
        logger.info('start shutdown for user %s', login)
        for command in commands:
            pretty_command = ' '.join(command)
            logger.debug('for user %s executig command %r', login, pretty_command)

            if not args.dry_run:
                execute(command)

                with open(cache_file, 'a') as file_:
                    print(repr(command), file=file_)


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    args.handler(args)
