import argparse


NANNY_SERVICES = ['prod/prestable:', '\twsproxy-vla', '\twsproxy-man', '\twsproxy-sas', '\twsproxy-prestable-sas',
                  'beta:', '\tuniproxy2_man', '\tuniproxy2_sas', '\tuniproxy2_vla']
NANNY_SERVICES_EXAMPLES = ['./wsproxy_admin --service wsproxy-vla,uniproxy2_man disconnect --ratio=0.33',
                           './wsproxy_admin --service wsproxy-vla disconnect --ratio=0.5']
UNIPROXY_HOST_EXAMPLES = ['./wsproxy_admin --host 12345.vla.yp-c.yandex.net,54321.sas.yp-c.yandex.net disconnect --ratio=1.0',
                          './wsproxy_admin --host 12345.vla.yp-c.yandex.net disconnect --ratio=0.1']
SERVICE_HELP_STR = '\t{}\nexamples:\n\t{}'.format(
    "\n\t".join(NANNY_SERVICES), "\n\t".join(NANNY_SERVICES_EXAMPLES))
HOST_HELP_STR = 'examples:\n\t{}'.format("\n\t".join(UNIPROXY_HOST_EXAMPLES))


class WsproxyArgparser:
    def __init__(self):
        parser = argparse.ArgumentParser(
            description='DOCUMENTATION: https://wiki.yandex-team.ru/voiceinfra/duty/wsproxyadmin/\n'
            + 'DESCRIPTION:\n\tone of service or host must be specified, specification of both parameters leads to failure'
            + '\n\nNANNY SERVICES:\n' + SERVICE_HELP_STR
            + '\n\nUNIPROXY HOSTS:\n' + HOST_HELP_STR,
            formatter_class=argparse.RawDescriptionHelpFormatter
        )
        sub_parsers = parser.add_subparsers(required=True)

        parser.add_argument('--service', type=str,
                            default=None, help='nanny services (separated with ,)')
        parser.add_argument('--host', type=str,
                            default=None, help='uniproxy hosts (separated with ,)')
        parser.add_argument('--apply-to-all', action='store_true',
                            help='apply operation to all specified uniproxy hosts')
        parser.add_argument('--batch-size', default=30,
                            help='batch size for application of operator (is ignored with --apply-to-all) default = 30')

        disconnect_parser = sub_parsers.add_parser('disconnect')
        disconnect_parser.set_defaults(action='disconnect_clients')
        disconnect_parser.add_argument('--surface', type=str, default=None)
        disconnect_parser.add_argument('--device-id', type=str, default=None)
        disconnect_parser.add_argument('--session-id', type=str, default=None)
        disconnect_parser.add_argument('--uuid', type=str, default=None)
        disconnect_parser.add_argument('--ratio', type=float, default=None,
                                       help='ratio of sessions to be operated (is required for disconnect without any arguments and with surface, other parameters are not premitted)')

        suspend_parser = sub_parsers.add_parser('suspend')
        suspend_parser.set_defaults(action='suspend_accepting')

        resume_parser = sub_parsers.add_parser('resume')
        resume_parser.set_defaults(action='resume_accepting')

        shutdown_parser = sub_parsers.add_parser('shutdown')
        shutdown_parser.set_defaults(action='shutdown')
        self.parser = parser
        self.parse_args = parser.parse_args
