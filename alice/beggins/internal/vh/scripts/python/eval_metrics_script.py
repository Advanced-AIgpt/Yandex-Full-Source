from yt.wrapper import YtClient
from sklearn.metrics import precision_score, recall_score, f1_score, roc_auc_score, accuracy_score, confusion_matrix
import pandas as pd
import numpy as np
import plotly.figure_factory as ff
from plotly.io import to_html


def _get_confusion_matrix_html(y_true, y_pred):
    tn, fp, fn, tp = confusion_matrix(y_true, y_pred).ravel()
    matrix = [[fp, fn], [tp, tn]]
    x_text = ['Positives', 'Negatives']
    y_text = ['False', 'True']
    matrix_text = [[f'{y} | {(y/len(y_true) * 100):.1f}%' for y in x] for x in matrix]

    matrix_plot = ff.create_annotated_heatmap(matrix, x=x_text, y=y_text, annotation_text=matrix_text, colorscale='Blues')

    matrix_plot.update_layout(margin=dict(l=10, r=10, t=50, b=10), font=dict(size=18))

    return to_html(matrix_plot, include_plotlyjs='cdn')


def _eval_metrics(threshold, table_rows):
    df = pd.DataFrame(table_rows)
    y_true, y_score = df['target'], df['score']
    y_pred = np.int32(y_score >= threshold)
    tn, fp, fn, tp = confusion_matrix(y_true, y_pred).ravel()
    html_code = _get_confusion_matrix_html(y_true, y_pred)
    return (
        {
            'precision': precision_score(y_true, y_pred),
            'recall': recall_score(y_true, y_pred),
            'f1': f1_score(y_true, y_pred),
            'roc_auc': roc_auc_score(y_true, y_score),
            'accuracy': accuracy_score(y_true, y_pred),
            'false_positive': fp.item(),
            'false_negative': fn.item(),
            'true_positive': tp.item(),
            'true_negative': tn.item(),
        },
        html_code
    )


def _read_mr_table(table_meta, token=None):
    yt_client = YtClient(proxy=table_meta['cluster'], token=token)
    table = table_meta['table']
    return yt_client.read_table(table)


def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    threshold = in1[0].get('threshold', 0.5)
    table_rows = _read_mr_table(mr_tables[0], token1)
    metrics, html_code = _eval_metrics(threshold, table_rows)
    html_file.write(html_code)
    return {
        'table': mr_tables[0],
        'threshold': threshold,
        'metrics': metrics,
    }
