import argparse
import asyncio
import logging
import re
import sys
from pprint import pformat

from aiohttp import ClientSession

logging.basicConfig(level=logging.INFO)


def get_logger(name):
    return logging.getLogger(name)


def make_pretty(input_data):
    return pformat(input_data)


def split_service_name(service: str):
    split_service = service.split('-')
    if len(split_service) == 1:
        split_service = service.split('_')
    assert len(split_service) > 4
    return split_service


def ask_user_ok(question: str = None) -> bool:
    print(question)
    ok = input('Is this OK? [y/N] ')
    return ok.lower() == 'y'


def fix_macro(macro: str):
    if not macro.startswith('_'):
        macro = '_' + macro
    if not macro.endswith('_'):
        macro = macro + '_'
    return macro


def clean_exit(code: int, session: ClientSession = None):
    if session:
        asyncio.get_event_loop().run_until_complete(session.close())
    sys.exit(code)


def make_parser() -> argparse.ArgumentParser:
    result_parser = argparse.ArgumentParser(description='Cli toolbox')

    subparsers = result_parser.add_subparsers(dest='used_parser', help='sub-commands')

    puncher_tools = subparsers.add_parser('puncher', aliases=['p'], help='puncher tools')
    nanny_tools = subparsers.add_parser('nanny', aliases=['n'], help='nanny tools')
    yplite_tools = subparsers.add_parser('yp', aliases=['y'], help='yp tools')

    # Category names
    puncher = puncher_tools.add_subparsers(dest='used_tool', help='copy tools')
    nanny = nanny_tools.add_subparsers(dest='used_tool', help='service clone')
    yplite = yplite_tools.add_subparsers(dest='used_tool', help='request podset')

    # Puncher
    puncher_copy = puncher.add_parser('copy', aliases=['cp'], help='copy puncher rules')
    puncher_copy.add_argument('source', type=str, help='source')
    puncher_copy.add_argument('destination', type=str, help='destination')
    puncher_copy.add_argument('comment', type=str, help='comment', nargs='*')

    # Nanny
    nanny_clone = nanny.add_parser('clone', aliases=['c'], help='clone service to other DC')
    nanny_clone.add_argument('service', type=str, help='nanny service id')
    nanny_clone.add_argument('category', type=str, help='nanny service category')
    nanny_clone.add_argument('--dest', type=str, help='custom destination service', default=None)
    nanny_clone.add_argument('-s', help='copy secrets (needs $YAV_TOKEN)', action='store_true', default=False)
    nanny_clone.add_argument('--no-dc', help='skip dc selection', action='store_true', default=False)
    nanny_clone.add_argument('description', type=str, help='human-readable description', nargs='*')

    nanny_sandbox = nanny.add_parser('sandbox', aliases=['sb'], help='set sandbox resources')
    nanny_sandbox.add_argument('service', type=str, help='nanny service id')
    nanny_sandbox.add_argument('--file', type=str, help='file with resources json', default='resources.json')
    nanny_sandbox.add_argument('--qloud', type=str, help='qloud service')

    nanny_tags = nanny.add_parser('tags', help='fix runtime and info tags')
    nanny_tags.add_argument('service', help='nanny service to fix')

    nanny_update_resource = nanny.add_parser('single', help='update single resource')
    nanny_update_resource.add_argument('service', type=str, help='nanny service id')
    nanny_update_resource.add_argument('--file', type=str, help='filename to load', default=None)
    nanny_update_resource.add_argument('--mode', type=str, help='resource_type', default='sandbox_files',
                                       choices=['sandbox_files', 'static_files', 'temp'])

    # Yplite
    yplite_mkpodset = yplite.add_parser('mkpodset', help='request podset in dc')
    yplite_mkpodset.add_argument('service', type=str, help='service name (without dc part)')
    yplite_mkpodset.add_argument('--cpu', type=int, help='cpu (milliseconds)', default=15)
    yplite_mkpodset.add_argument('--mem', type=int, help='memory (megabytes)', default=40960)
    yplite_mkpodset.add_argument('--disk', type=int, help='disk size (megabytes)', default=30720)
    yplite_mkpodset.add_argument('--count', type=int, help='pod count', default=1)
    yplite_mkpodset.add_argument('--net', type=str, help='network macro', default='YALDI_PROD_NETS')
    yplite_mkpodset.add_argument('--abc', type=int, help='ABC service id', default=2728)
    yplite_mkpodset.add_argument('--dc', type=str, help='datacenters', nargs='+', default='all',
                                 choices=('man', 'vla', 'sas', 'all'))

    yplite_mkendpoints = yplite.add_parser('mkendpoint', help='request endpoint sets')
    yplite_mkendpoints.add_argument('service', type=str, help='service name (without dc part)')
    yplite_mkendpoints.add_argument('--dc', type=str, help='datacenters', nargs='+', default='all',
                                    choices=('man', 'vla', 'sas', 'all'))
    return result_parser


def make_service_template(service: str) -> str:
    return re.sub('[_-](man|vla|sas)$', '-{}', service)
