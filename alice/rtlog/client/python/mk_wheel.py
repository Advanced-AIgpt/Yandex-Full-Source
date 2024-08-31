import argparse
import subprocess
import shutil
import os
import sys
import tempfile
from configparser import ConfigParser
from twine.commands.upload import main as twine_upload


PL_LINUX = 'manylinux1_x86_64'
PL_MACOS = 'macosx_10_6_intel.macosx_10_9_intel.macosx_10_9_x86_64.macosx_10_10_intel.macosx_10_10_x86_64'
PL_WIN = 'win_amd64'


def get_arcadia_root():
    arcadia_root_candidate = os.path.dirname(os.path.abspath(__file__))
    retries_count = 10
    for _ in range(retries_count):
        if os.path.exists(os.path.join(arcadia_root_candidate, '.arcadia.root')):
            return arcadia_root_candidate
        arcadia_root_candidate = os.path.dirname(arcadia_root_candidate)
    raise RuntimeError("Unable to find .arcadia.root!")


def get_version(version_py):
    exec(compile(open(version_py, "rb").read(), version_py, 'exec'))
    return locals()['VERSION']


def gen_platform():
    import distutils.util

    value = distutils.util.get_platform().replace("linux", "manylinux1")
    value = value.replace('-', '_').replace('.', '_')
    if 'macosx' in value:
        value = PL_MACOS
    return value


def pipy_upload(whl_path):
    #https://wiki.yandex-team.ru/pypi/
    pypi_config = '~/.pypirc'

    config_parser = ConfigParser()
    config_parser.read(os.path.expanduser(pypi_config))
    args = [
        '--repository-url', config_parser.get('yandex', 'repository'),
        '-u', config_parser.get('yandex', 'username'),
        '-p', config_parser.get('yandex', 'password'),
        whl_path
    ]
    twine_upload(args)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--dry-run', dest='dry_run',
        required=False, default=False, action='store_true',
    )
    args = parser.parse_args()

    arc_root = get_arcadia_root()
    pkg_name = 'rtlog'
    client_source_path = os.path.join(arc_root, 'alice', 'rtlog', 'client', 'python')
    ver = get_version(os.path.join(client_source_path, 'version.py'))
    target_platform = gen_platform()
    target_wheel_path = tempfile.mkdtemp()
    if sys.version_info.major == 2:
        py_config = 'python-config'
        lang = 'cp27'
    else:
        py_config = 'python3-config'
        lang = 'cp3' + str(sys.version_info.minor)
    wheel_name = os.path.join(client_source_path,
                              '{}-{}-{}-none-{}.whl'.format(pkg_name, ver, lang, target_platform))

    def create_module_directory(path):
        module_directory_path = os.path.join(target_wheel_path, path)
        if not os.path.exists(module_directory_path):
            os.mkdir(module_directory_path)
        if not os.path.exists(os.path.join(module_directory_path, '__init__.py')):
            open(os.path.join(module_directory_path, '__init__.py'), 'a').close()
        return module_directory_path

    def deploy_rtlog():
        source_path = os.path.join(client_source_path, 'rtlog')
        cmd = [
            sys.executable,
            arc_root + '/ya', 'make',
            source_path,
            '-r',
            '-j150',
            '-DPYTHON_CONFIG=' + py_config,
            '-DUSE_ARCADIA_PYTHON=no',
            '-DOS_SDK=local',
        ]
        print(' '.join(cmd))
        subprocess.check_call(cmd)
        module_path = create_module_directory('rtlog')
        shutil.copy(os.path.join(source_path, 'client.so'), module_path)
        shutil.copy(os.path.join(source_path, '__init__.py'), module_path)
        shutil.copy(os.path.join(source_path, 'thread_local.py'), module_path)
        shutil.copy(os.path.join(source_path, '_handler.py'), module_path)

    def deploy_proto(source_path):
        source_full_path = os.path.join(arc_root, source_path)
        cmd = [
            sys.executable,
            arc_root + '/ya', 'make',
            os.path.join(source_full_path, 'python'),
            '-r',
            '-j150',
            '--checkout'
        ]
        print(' '.join(cmd))
        subprocess.check_call(cmd)

        target_path = source_path
        parts = target_path.split('/')
        for p_size in range(1, len(parts) + 1):
            create_module_directory(os.path.join(*tuple(parts[:p_size])))
        for f in os.listdir(source_full_path):
            if f.endswith('_pb2.py'):
                shutil.copy(os.path.join(source_full_path, f),
                            os.path.join(target_wheel_path, target_path, f))

    def substitute_vars(file_path):
        with open(file_path, 'r') as fm:
            metadata = fm.read()
        metadata = metadata.format(pkg_name=pkg_name, version=ver)
        with open(file_path, 'w') as fm:
            fm.write(metadata)

    deploy_rtlog()
    deploy_proto('library/cpp/eventlog/proto')
    deploy_proto('alice/rtlog/protos')

    # Create metadata
    dist_info_dir = os.path.join(target_wheel_path, '{}-{}.dist-info'.format(pkg_name, ver))
    shutil.copytree(os.path.join(client_source_path, 'rtlog.dist-info'), dist_info_dir)
    substitute_vars(os.path.join(dist_info_dir, 'METADATA'))
    substitute_vars(os.path.join(dist_info_dir, 'top_level.txt'))

    # Create wheel
    shutil.make_archive(wheel_name, 'zip', target_wheel_path)
    shutil.move(wheel_name + '.zip', wheel_name)
    print(wheel_name)
    if args.dry_run:
        print(target_wheel_path)
    else:
        shutil.rmtree(target_wheel_path)
        pipy_upload(wheel_name)


if __name__ == '__main__':
    main()
