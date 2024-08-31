import argparse
import re


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('in_path', nargs='+')
    parser.add_argument('--out', dest='out_path', default='/dev/stdout')
    return parser.parse_args()


def relpath(path):
    return re.sub('.*/alice/hollywood/shards/common/prod/fast_data/alice_show/config/', '', path)


def main():
    args = parse_args()
    config = "# built from " + ", ".join(relpath(path) for path in args.in_path) + "\n"
    for in_path in args.in_path:
        with open(in_path, 'r') as fobj:
            name = relpath(in_path)
            content = fobj.read()
            config += "\n# " + name + "\n"
            if re.match(r"phrases.*", name):
                config += "PhrasesCorpus {\n" + content + "}\n"
            elif re.match(r"conditions.*", name):
                config += "TagConditionsCorpus {\n" + content + "}\n"
            else:
                config += content

    with open(args.out_path, 'w') as fobj:
        fobj.write(config)
