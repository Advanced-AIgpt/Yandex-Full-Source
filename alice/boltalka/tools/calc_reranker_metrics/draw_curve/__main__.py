import argparse
import json
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from collections import defaultdict


def parse_model_params(params):
    return dict([x.split('=') for x in params.split(';')])


def parse_plot_data(filename, x_name, y_name, select_by=None):
    with open(filename, 'r') as f:
        reports = json.load(f)
    points = defaultdict(list)
    for m in reports:
        point = (m[x_name], m[y_name])
        if select_by:
            model_params = parse_model_params(m['model'])
            points[model_params[select_by]].append(point)
        else:
            points[''].append(point)
    points = {k: sorted(v) for k, v in points.items()}
    return points


def main(args):
    filenames = args.input.split(',')
    labels = args.labels.split(',')
    assert len(filenames) == len(labels)
    for filename, label in zip(filenames, labels):
        plot_data = parse_plot_data(filename, args.x_name, args.y_name, args.select_by)
        for label_spec, points in plot_data.items():
            x, y = zip(*points)
            if label_spec:
                label_total = label + '_' + label_spec
            else:
                label_total = label
            plt.plot(x, y, '-o', label=label_total)
    plt.xlabel(args.x_name)
    plt.ylabel(args.y_name)
    plt.legend(loc="lower left", prop={'size': 8})
    plt.savefig(args.output, format='png')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--input', required=True)
    parser.add_argument('--labels', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--x-name', required=True)
    parser.add_argument('--y-name', required=True)
    parser.add_argument('--select-by', default=None)
    args = parser.parse_args()
    main(args)
