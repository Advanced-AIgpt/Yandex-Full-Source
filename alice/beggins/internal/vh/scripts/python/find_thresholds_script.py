from yt.wrapper import YtClient
from bisect import bisect_left
import sklearn.metrics
import pandas as pd
import numpy as np
import plotly.express as px
from plotly.subplots import make_subplots
from plotly.io import to_html
from enum import Enum


def add_precision_recall_plot(plots, precision, recall, thresholds, x_threshold):
    plot = px.line(
        data_frame=pd.DataFrame({'precision': precision, 'recall': recall, 'thresholds': thresholds}),
        x='recall',
        y='precision',
        hover_data=['thresholds'],
    )
    plots.add_trace(plot['data'][0], row=1, col=1)
    plots.add_shape(type='line', x0=x_threshold, y0=0, x1=x_threshold, y1=1, row=1, col=1)
    plots.update_xaxes(title_text='recall', row=1, col=1)
    plots.update_yaxes(title_text='precision', row=1, col=1)


def add_roc_plot(plots, fpr, tpr, thresholds, x_threshold):
    plot = px.line(
        data_frame=pd.DataFrame({'tpr': tpr, 'fpr': fpr, 'thresholds': thresholds}),
        x='fpr',
        y='tpr',
        hover_data=['thresholds'],
    )
    plots.add_trace(plot['data'][0], row=2, col=1)
    plots.add_shape(type='line', x0=x_threshold, y0=0, x1=x_threshold, y1=1, row=2, col=1)
    plots.update_xaxes(title_text='fpr', row=2, col=1)
    plots.update_yaxes(title_text='tpr', row=2, col=1)


def add_tpr_plot(plots, tpr, thresholds, x_threshold):
    plot = px.line(
        data_frame=pd.DataFrame({'tpr': tpr, 'thresholds': thresholds}),
        x='thresholds',
        y='tpr',
    )
    plots.add_trace(plot['data'][0], row=1, col=2)
    plots.add_shape(type='line', x0=x_threshold, y0=0, x1=x_threshold, y1=1, row=1, col=2)
    plots.update_xaxes(title_text='threshold', row=1, col=2)
    plots.update_yaxes(title_text='tpr', row=1, col=2)


def add_fpr_plot(plots, fpr, thresholds, x_threshold):
    plot = px.line(
        data_frame=pd.DataFrame({'fpr': fpr, 'thresholds': thresholds}),
        x='thresholds',
        y='fpr',
    )
    plots.add_trace(plot['data'][0], row=2, col=2)
    plots.add_shape(type='line', x0=x_threshold, y0=0, x1=x_threshold, y1=1, row=2, col=2)
    plots.update_xaxes(title_text='threshold', row=2, col=2)
    plots.update_yaxes(title_text='fpr', row=2, col=2)


def get_threshold_plot_html(y_true: np.ndarray, y_score: np.ndarray, threshold: float):
    precision, recall, precision_recall_thresholds = sklearn.metrics.precision_recall_curve(y_true, y_score)

    precision = precision[:-1]
    recall = recall[:-1]

    fpr, tpr, roc_thresholds = sklearn.metrics.roc_curve(y_true, y_score)

    recall_value_position = bisect_left(precision_recall_thresholds, threshold)
    reversed_fpr_value_position = bisect_left(list(reversed(roc_thresholds)), threshold)
    recall_value = recall[recall_value_position]
    fpr_value = list(reversed(fpr))[reversed_fpr_value_position]

    threshold_plots = make_subplots(rows=2, cols=2, subplot_titles=('Precision-recall', 'Tpr', 'Roc', 'Fpr'))

    add_precision_recall_plot(threshold_plots, precision, recall, precision_recall_thresholds, x_threshold=recall_value)
    add_roc_plot(threshold_plots, fpr, tpr, roc_thresholds, x_threshold=fpr_value)
    add_tpr_plot(threshold_plots, tpr, roc_thresholds, x_threshold=threshold)
    add_fpr_plot(threshold_plots, fpr, roc_thresholds, x_threshold=threshold)

    threshold_plots.update_layout(title_text=f'Recommended threshold: {threshold}',
                                  title_font_size=30,
                                  title_x=.5,
                                  hovermode='closest',
                                  xaxis=dict(hoverformat='.4f'),
                                  yaxis=dict(hoverformat='.4f'))

    return to_html(threshold_plots, include_plotlyjs='cdn')


def f_score(precision: np.ndarray, recall: np.ndarray, beta: float) -> float:
    b2 = beta * beta
    numerator = (1 + b2) * precision * recall
    denominator = b2 * precision + recall

    if denominator == 0.:
        return 0.

    return numerator / denominator


class ThresholdStrategy(Enum):
    max_f1_no_conditions = 'max_f1_no_conditions'
    max_precision_condition_recall = 'max_precision_condition_recall'
    max_f1_condition_recall = 'max_f1_condition_recall'


def find_bound_threshold_index(recall: np.ndarray, recall_threshold: float) -> int:
    for i in range(len(recall) - 1):
        if recall[i] <= recall_threshold:
            return i

    return len(recall) - 2


def get_threshold_max_f1_condition_recall(precision: np.ndarray, recall: np.ndarray, thresholds: np.ndarray, recall_threshold: float):
    if len(thresholds) == 1:
        return thresholds[0]

    threshold_index = find_bound_threshold_index(recall, recall_threshold)
    f1_scores = [f_score(*value, beta=1) for value in zip(precision, recall)]
    return thresholds[np.argmax(f1_scores[:threshold_index])]


def get_threshold_max_precision_condition_recall(precision: np.ndarray, recall: np.ndarray, thresholds: np.ndarray, recall_threshold: float):
    if len(thresholds) == 1:
        return thresholds[0]

    threshold_index = find_bound_threshold_index(recall, recall_threshold)
    return thresholds[np.argmax(precision[:threshold_index])]


def get_threshold_max_f1_no_condition(precision: np.ndarray, recall: np.ndarray, thresholds: np.ndarray):
    f1_scores = [f_score(*value, beta=1) for value in zip(precision, recall)]
    return thresholds[np.argmax(f1_scores)]


def get_threshold(precision: np.ndarray, recall: np.ndarray, thresholds: np.ndarray,
                  recall_threshold: float, threshold_strategy: ThresholdStrategy):
    threshold = 0
    if threshold_strategy == ThresholdStrategy.max_f1_no_conditions:
        threshold = get_threshold_max_f1_no_condition(precision, recall, thresholds)
    elif threshold_strategy == ThresholdStrategy.max_f1_condition_recall:
        threshold = get_threshold_max_f1_condition_recall(precision, recall, thresholds, recall_threshold)
    elif threshold_strategy == ThresholdStrategy.max_precision_condition_recall:
        threshold = get_threshold_max_precision_condition_recall(precision, recall, thresholds, recall_threshold)
    else:
        raise RuntimeError('bad threshold strategy')

    return threshold


def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    table_meta = mr_tables[0]
    params = in1[0] if in1 else {}

    yt_client = YtClient(proxy=table_meta['cluster'], token=token1)
    table = table_meta['table']

    threshold_strategy = ThresholdStrategy(params.get('threshold_selection_strategy', 'max_precision_condition_recall'))
    recall_threshold = params.get('recall_threshold', 0.8)
    beta_logging_threshold = params.get('beta_logging_threshold', 2.0)

    if recall_threshold == 1.:
        raise RuntimeError('Recall threshold lies in [0, 1)')

    df = pd.DataFrame(yt_client.read_table(table))
    y_true, y_score = df['target'], df['score']
    precision, recall, thresholds = sklearn.metrics.precision_recall_curve(y_true, y_score)

    threshold = get_threshold(precision, recall, thresholds, recall_threshold, threshold_strategy)

    scores = [f_score(*value, beta=beta_logging_threshold) for value in zip(precision, recall)]
    logging_threshold = thresholds[np.argmax(scores)]

    y_pred = np.int32(y_score >= threshold)
    y_logging_pred = np.int32(y_score >= logging_threshold)

    # save plot to html
    html_code = get_threshold_plot_html(y_true, y_score, threshold)
    html_file.write(html_code)

    return {
        'threshold': threshold,
        'logging_threshold': logging_threshold,
        'threshold_metrics': {
            'precision': sklearn.metrics.precision_score(y_true, y_pred),
            'recall': sklearn.metrics.recall_score(y_true, y_pred),
            'f1': sklearn.metrics.f1_score(y_true, y_pred),
        },
        'logging_threshold_metrics': {
            'precision': sklearn.metrics.precision_score(y_true, y_logging_pred),
            'recall': sklearn.metrics.recall_score(y_true, y_logging_pred),
            'f1': sklearn.metrics.f1_score(y_true, y_logging_pred),
        },
        'params': {
            'recall_threshold': recall_threshold,
            'beta_logging_threshold': beta_logging_threshold,
            'threshold_strategy': str(threshold_strategy)
        },
    }
