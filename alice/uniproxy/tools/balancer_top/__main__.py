#!/usr/bin/env python3
import asyncio
import logging
import argparse
import time
import sys
import os


# -------------------------------------------------------------------------------------------------
if getattr(sys, "is_standalone_binary", False):
    from .nanny import get_service_hostnames
    from .collect_logs import collect_logs
    from .top import create_top

    LOG_PARSER_BIN_PATH = os.path.join(os.path.dirname(sys.argv[0]), "balancer_log_parser/blp")
else:
    from nanny import get_service_hostnames
    from collect_logs import collect_logs
    from top import create_top

    LOG_PARSER_BIN_PATH = os.path.join(os.path.dirname(__file__), "balancer_log_parser/blp")


# -------------------------------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-v", "--verbose", action="store_true", help="Enable debug output")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--nanny-service", type=str, help="Collect logs on instances of this Nanny service")
    group.add_argument("--host-list", type=str, nargs="+", help="Collect logs on these hosts")

    parser.add_argument("--log-file", type=str, help="Log file on target hosts",
                        default="/logs/current-access_log-balancer-443")
    parser.add_argument("--host", type=str, help="Filter out requests which \"Host\" headers aren't equal to this")
    parser.add_argument("--uri", type=str, help="Filter out requests which URIs don't start with this")
    parser.add_argument("--begin-ts", type=str, help="Filter out requests that made before this UTC timestamp")
    parser.add_argument("--end-ts", type=str, help="Filter out requests that made after this UTC timestamp")
    parser.add_argument("--max", type=int, help="Limit amount of records collected from each host")
    parser.add_argument("--timeout", type=int, default=60, help="Timeout in seconds")

    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    if args.nanny_service is not None:
        hosts = get_service_hostnames(args.nanny_service)
    else:
        hosts = args.host_list
    logging.info(f"Collect logs from {len(hosts)} hosts, ")

    if args.begin_ts is not None and args.begin_ts.startswith("-"):
        args.begin_ts = time.time() - int(args.begin_ts[1:])
    if args.end_ts is not None and args.end_ts.startswith("-"):
        args.end_ts = time.time() - int(args.end_ts[1:])
    logging.debug(f"Collect record in interval [{args.begin_ts}, {args.end_ts}]")

    records = asyncio.run(collect_logs(
        hosts,
        parser_bin=LOG_PARSER_BIN_PATH,
        timeout=args.timeout,
        path=args.log_file,
        begin_ts=args.begin_ts,
        end_ts=args.end_ts,
        uri=args.uri,
        max=args.max
    ))

    logging.info(f"Collected {len(records)} records")
    with open("dump", "w") as f:
        f.writelines(records)

    create_top(records)


if __name__ == "__main__":
    main()
