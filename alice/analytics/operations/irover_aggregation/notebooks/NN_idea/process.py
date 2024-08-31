import argparse
import json

def main(args):
    with open(args.out_path,'w+') as outf:
        for path in args.in_paths:
            with open(path) as inf:
                for line in inf:
                    line_parsed=json.loads(line.strip())['raw_assesments']
                    if line_parsed:
                            for assesment in line_parsed:
                                print(assesment['text'].lower().replace('ё','е'),file=outf)

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--in-paths',nargs='+',required=True)
    parser.add_argument('--out-path',required=True)
    args=parser.parse_args()
    main(args)
