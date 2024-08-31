from alice.tools.nanny_tool.lib.Nanny import Nanny, get_resource_versions
from alice.tools.nanny_tool.lib.Puncher import Puncher
from alice.tools.nanny_tool.lib.Yp import Yp
from alice.tools.nanny_tool.lib.nanny_fix_tags import fix_tags_by_service
from alice.tools.nanny_tool.lib.tools import make_parser, get_logger

log = get_logger(__name__)

if __name__ == '__main__':
    parser = make_parser()
    args = parser.parse_args()
    if args.used_parser in ['puncher', 'p']:
        p = Puncher()
        if args.used_tool in ['copy', 'cp']:
            p.copy_puncher_rules(args.source, args.destination, ' '.join(args.comment))
        else:
            parser.print_help()

    elif args.used_parser in ['nanny', 'n']:
        n = Nanny()
        if args.used_tool in ['clone', 'c']:
            n.clone_service(args.service, args.category, ' '.join(args.description), args.dest, args.s, args.no_dc)
        elif args.used_tool in ['sandbox', 'sb']:
            if args.qloud is None:
                n.modify_nanny_resources(args.service, get_resource_versions(filename=args.file))
            else:
                n.modify_nanny_resources(args.service, get_resource_versions(qloud_app=args.qloud))
        elif args.used_tool == 'tags':
            fix_tags_by_service(args.service)
        elif args.used_tool == 'single':
            n.modify_single_resource(args.service, args.file, args.mode)
        else:
            parser.print_help()
    elif args.used_parser in ['yp', 'y']:
        y = Yp()
        if args.used_tool in ['mkpodset']:
            y.create_pod_set(args.cpu,
                             args.mem,
                             args.disk,
                             args.count,
                             args.service,
                             args.net,
                             args.dc,
                             args.abc)
        elif args.used_tool in ['mkendpoint']:
            y.create_endpoint_sets(args.service, args.dc)
        else:
            parser.print_help()

    else:
        parser.print_help()
