from matplotlib import pyplot as plt
plt.switch_backend('agg')
import numpy as np
import subprocess
import argparse
import pandas

def show_plot(datas, x, y, no_grid):
    if not no_grid:
        plt.grid()
    p = subprocess.Popen('/home/nzinov/.iterm2/imgcat', stdin=subprocess.PIPE, close_fds=True)
    for fname, data in datas:
        plt.plot(data[:, x], data[:, y], label=fname)
    plt.legend()
    if x == 0:
        plt.xlim((0, 10))
    plt.savefig(p.stdin)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-x', type=int, default=0)
    parser.add_argument('-y', type=int, default=1)
    parser.add_argument('-s', type=str, default=' ')
    parser.add_argument('--no-grid', action='store_true')
    parser.add_argument('file', nargs='*')
    args = parser.parse_args()
    data = [(f, pandas.read_csv(f, header=None, sep=args.s).values) for f in args.file]
    show_plot(data, args.x, args.y, args.no_grid)
