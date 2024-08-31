import json
import types
import argparse

from code import main as code_main


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--in1', nargs='*', default=[])
    parser.add_argument('--in2', nargs='*', default=[])
    parser.add_argument('--in3', nargs='*', default=[])
    parser.add_argument('--mr', nargs='*', default=[])
    parser.add_argument('--yt_read', action='store_true')
    parser.add_argument('--out1')
    parser.add_argument('--out2')
    parser.add_argument('--html_report')
    parser.add_argument('--token1')
    parser.add_argument('--token2')
    parser.add_argument('--param1')
    parser.add_argument('--param2')
    return parser.parse_args()


def parse_inputs(args):
    inputs = dict(in1=[], in2=[], in3=[], mr_tables=[])
    for filename in args.in1:
        with open(filename, encoding='utf-8') as f:
            data = json.load(f)
            inputs['in1'] += data if isinstance(data, list) else [data]

    for filename in args.in2:
        with open(filename, encoding='utf-8') as f:
            data = json.load(f)
            inputs['in2'] += data if isinstance(data, list) else [data]

    for filename in args.in3:
        with open(filename, encoding='utf-8') as f:
            data = json.load(f)
            inputs['in3'] += data if isinstance(data, list) else [data]

    for filename in args.mr:
        with open(filename, encoding='utf-8') as f:
            inputs['mr_tables'].append(json.load(f))

    if args.yt_read and inputs['mr_tables'] and args.token1:
        # read yt tables to in1, in2, in3 instead of intended json, using token1 as yt_token
        import yt.wrapper as yt
        yt.config.config['token'] = args.token1
        for mr_table, input_name in zip(inputs['mr_tables'][:3], ['in1', 'in2', 'in3']):
            yt.config.config['proxy']['url'] = mr_table['cluster']
            inputs[input_name] = yt.read_table(mr_table['table'], format='<encode_utf8=false>json')
    return inputs


def prepare_code_args(args):
    code_args = {}
    for arg in ['token1', 'token2', 'param1', 'param2']:
        if hasattr(args, arg):
            code_args[arg] = getattr(args, arg)
    return code_args


def save_results(result, args):
    if result is not None:
        result1, result2 = None, None
        if isinstance(result, types.GeneratorType):
            result = list(result)
            if isinstance(result[0], tuple) and args.out2:
                # split generator yield tuple to two lists
                result1, result2 = zip(*result)
            else:
                result1 = result
        elif isinstance(result, tuple) and args.out2:
            # split function return tuple to two lists
            result1 = result[0]
            result2 = result[1]
        else:
            result1 = result

        if result1 and args.out1:
            with open(args.out1, 'w', encoding='utf-8') as out1_file:
                out1_file.write(json.dumps(result1, ensure_ascii=False, indent=4))

        if result2 and args.out2:
            with open(args.out2, 'w', encoding='utf-8') as out2_file:
                out2_file.write(json.dumps(result2, ensure_ascii=False, indent=4))


def main():
    args = parse_args()
    inputs = parse_inputs(args)
    code_args = prepare_code_args(args)

    html_file = None
    if args.html_report:
        html_file = open(args.html_report, 'w', encoding='utf-8')
    code_args['html_file'] = html_file

    result = code_main(inputs['in1'], inputs['in2'], inputs['in3'], inputs['mr_tables'], **code_args)

    save_results(result, args)

    if args.html_report:
        html_file.close()


if __name__ == '__main__':
    main()
