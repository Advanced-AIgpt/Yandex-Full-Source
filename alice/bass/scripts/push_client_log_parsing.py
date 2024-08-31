#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import datetime
import json
import os
import re
import sys
import traceback

main_regex = re.compile('^(?P<xyz>\d+;\d+;\d+;)?(?P<type>[A-Z]+)\:\ (?P<time>\d\d\d\d\-\d\d\-\d\d\ \d\d\:\d\d\:\d\d\.\d\d\d\ [+-]?\d\d\d\d)\ (?P<file>.+\..{1,3})\:(?P<line>\d+)\ \<(?P<reqid>[A-Za-z0-9_\-]*)\+(?P<hypothesis_number>[\-0-9]*)\>\ (?P<content>.*)$')
aux_regex = re.compile('^(?P<xyz>\d+;\d+;\d+;)?(?P<content>.*)$')

def find_command_structure(input_str):
    if input_str:
        result = dict()
        main_match = main_regex.match(str(input_str))
        if main_match:
            result = {k:(main_match.group(k) or '') for k in ['xyz', 'type', 'time', 'file', 'line', 'reqid', 'hypothesis_number', 'content']}
            result['found'] = True
        else:
            aux_match = aux_regex.match(str(input_str))
            if aux_match:
                result = {k:(aux_match.group(k) or '') for k in ['xyz', 'content']}
        return result

def handle_pipe_input(args):
    current_command_structure = dict()
    strings = []
    while True:
        line = sys.stdin.readline()
        if line == '':
            break
        status = find_command_structure(line)
        if status.get('found'):
            print_item(current_command_structure, strings, args)
            current_command_structure = status
            strings = [ status['content'] ]
        else:
            current_command_structure['xyz'] = status['xyz']
            strings.append(status['content'])
    print_item(current_command_structure, strings, args)

def is_valid_command_structure(command_structure):
    return all (must_have_key in command_structure for must_have_key in ('type', 'time', 'file', 'line', 'reqid', 'hypothesis_number'))

def print_item(command_structure, strings, args):
    if command_structure and is_valid_command_structure(command_structure):
        all_content = [item.expandtabs(8) for item in strings]
        combined_line = "\\n".join(all_content)

        xyz = command_structure.get('xyz', '')

        if args.output_format == "tskv":
            # TODO(akhruslan): drop "hypothesys_number" field because of misspell
            res = '{xyz}tskv\ttskv_format=bass-vins-log-tskv\tenv={env_machine}\tnanny_service_id={nanny_service_id}\ttype={type_field}\ttimestamp={time}\tfile={file_field}\tline={line}\treqid={reqid}\thypothesis_number={hypothesis_number}\thypothesys_number={hypothesis_number}\tcontent={content}'.format(
                xyz=xyz,
                env_machine=args.env,
                nanny_service_id=args.nanny,
                type_field=command_structure['type'],
                time=command_structure['time'],
                file_field=command_structure['file'],
                line=command_structure['line'],
                reqid=command_structure['reqid'],
                hypothesis_number=command_structure['hypothesis_number'],
                content=combined_line
            )
        elif args.output_format == "json":
            j = {i: command_structure[i] for i in ('type', 'time', 'reqid')}
            j['env'] = args.env
            j['nanny_service_id'] = args.nanny
            j['content'] = combined_line

            res = json.dumps(j, ensure_ascii=False)

        print(res)
        sys.stdout.flush()

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--env", help="Current environment (usually: production or testing)", default="undefined")
    parser.add_argument("--nanny", help="Current nanny service", default=os.environ['NANNY_SERVICE_ID'])
    parser.add_argument("--output-format", help="Output logs format: 'tskv' or 'json'", default='tskv')

    return parser.parse_args()

def main():
    try:
        handle_pipe_input(parse_args())
    except BaseException as e:
        with open('/logs/pipe_error.txt', 'a') as error_file:
            error_file.write("[{}] ".format(datetime.datetime.utcnow().isoformat()))
            error_file.write("Exception during handle_pipe_input: {}\n".format(str(e)))
            e_type, e_value, e_tb = sys.exc_info()
            error_file.write(''.join(traceback.format_exception(etype=e_type, value=e_value, tb=e_tb)))


if __name__ == "__main__":
    main()
