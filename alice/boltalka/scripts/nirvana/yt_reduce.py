#!/usr/bin/python
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

class Reduce(object):
    def __init__(self, reduce_expr, prereduce_expr, postreduce_expr, start_expr):
        self.reduce_expr = compile(reduce_expr, '<string>', 'exec')
        self.prereduce_expr = compile(prereduce_expr, '<string>', 'exec')
        self.postreduce_expr = compile(postreduce_expr, '<string>', 'exec')
        self.start_expr = compile(start_expr, '<string>', 'exec')

    def start(self):
        exec(self.start_expr)

    def __call__(self, key, rows):
        result = []
        exec self.prereduce_expr in globals(), locals()
        for row in rows:
            exec self.reduce_expr in globals(), locals()
        exec self.postreduce_expr in globals(), locals()
        for row in result:
            yield row

def main(args):
    reducer = Reduce(args.reduce_expr, args.prereduce_expr, args.postreduce_expr, args.start_expr)
    yt.run_reduce(reducer, args.src, args.dst, reduce_by=args.reduce_by.split(','))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--reduce-by', required=True)
    parser.add_argument('--reduce-expr', required=True)
    parser.add_argument('--prereduce-expr', default='')
    parser.add_argument('--postreduce-expr', default='')
    parser.add_argument('--start-expr', default='')
    args = parser.parse_args()
    main(args)
