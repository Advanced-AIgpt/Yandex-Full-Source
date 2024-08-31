from argparse import ArgumentParser
import json
import sys
from sklearn import metrics
import matplotlib.pyplot as plt
import numpy as np


def __mse(y_true, y_pred):
    return metrics.mean_squared_error(y_true, y_pred)


def __rmse(y_true, y_pred):
    mse = __mse(y_true, y_pred)
    return mse ** .5


def __log_loss(y_true, y_pred, eps=1e-15):
    return metrics.log_loss(y_true, y_pred)


def __read(filename):
    Y = np.loadtxt(filename, delimiter='\t', usecols=(1, 4))
    y_true, y_pred = np.hsplit(Y, 2)
    return y_true, y_pred


def __score(y_true, y_pred, loss_type):
    if loss_type == 'mse':
        loss = __mse(y_true, y_pred)
    if loss_type == 'rmse':
        loss = __rmse(y_true, y_pred)
    if loss_type == 'log-loss':
        loss = __log_loss(y_true, y_pred)
    return {
        'lossType': loss_type,
        'loss': loss,
        'size': len(y_true),
    }


def __roc_plot(y_true, y_pred, roc_auc, ax=None):
    if ax is None:
        ax = plt.gca()
    fpr, tpr, _ = metrics.roc_curve(y_true, y_pred, pos_label=1.)
    ax.set_xlabel('fpr')
    ax.set_ylabel('tpr')
    ax.set_ylim([0.0, 1.05])
    ax.set_xlim([0.0, 1.0])
    ax.plot(fpr, tpr)
    ax.set_title('ROC-AUC curve: AUC={0:0.2f}'.format(roc_auc))


def __roc_auc(y_true, y_pred, plot_filename=None):
    roc_auc = metrics.roc_auc_score(y_true, y_pred)
    if plot_filename is not None:
        plt.ioff()
        __roc_plot(y_true, y_pred, roc_auc)
        plt.savefig(plot_filename)
    return roc_auc


def __write(filename, score):
    with open(filename, 'w') as f:
        json.dump(score, f)


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument('--input', required=True)
    arg_parser.add_argument('--output')
    arg_parser.add_argument('--output-roc-plot')
    arg_parser.add_argument('--loss-type', required=True, choices=['mse', 'rmse', 'log-loss'])
    arg_parser.add_argument('--dataset-name')
    arg_parser.add_argument('--one-class', default=False)
    args = arg_parser.parse_args()

    y_true, y_pred = __read(args.input)
    if args.one_class:
        print("To support one-class prediction added two fake records: ")
        max = np.amax(y_pred)
        y_true = np.append(y_true, 1.0)
        y_pred = np.append(y_pred, max)
        print("1.0 ", max)

        y_true = np.append(y_true, 0.0)
        min = np.amin(y_pred)
        y_pred = np.append(y_pred, min)
        print("0.0 ", min)
    print(y_true.shape, y_pred.shape, file=sys.stderr)
    score = __score(y_true, y_pred, args.loss_type)
    score.update({'roc_auc': __roc_auc(y_true, y_pred, args.output_roc_plot)})
    if args.dataset_name:
        score.update({'dataset': args.dataset_name})
    if args.output:
        __write(args.output, score)
    print(score)


if __name__ == "__main__":
    main()
