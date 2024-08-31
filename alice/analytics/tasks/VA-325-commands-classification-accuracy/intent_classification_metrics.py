#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation

import datetime
from collections import Counter
import pandas as pd
from sklearn.metrics import accuracy_score, f1_score, precision_score, recall_score


def main(dataset):
    """
        INPUT: dataset - a list of dictionaries {'true_label' -> string, 'pred_label': str}
        OUTPUT:
            list with a single dict('counters' -> dict(
                                        'fielddate' -> string (todays date in datetime.date.isoformat ("yyyy-MM-dd")),
                                        'true_class_counts' -> dict(
                                                            intent -> number of true occurrences with that intent
                                                        ),
                                        'pred_class_counts' - > dict(
                                                            intent -> number of predicted occurrences with that intent
                                                        )
                                    ),
                                    'metrics' -> list of dict(
                                         'value' -> float (some metric),
                                         'metric' -> string (type of metric; ex: 'accuracy'),
                                         'average' -> string (averaging; see scikit-learn.metrics docs),
                                         'intent' -> string,
                                         'fielddate' -> string
                                        )
                                    )

        list over the top dictionary is added for convenient use in nirvana
    """

    # save all unique intents into a set (set of pred_labels and true_labels are not necessarily the same)
    all_classes = set()
    for item in dataset:
        all_classes.add(item['pred_label'])
        all_classes.add(item['true_label'])

    # store mapping [intent to integer id] (and its inverse)
    classname2id = {class_: i for i, class_ in enumerate(all_classes)}
    id2classname = {v: k for k, v in classname2id.items()}

    # use pandas DataFrame for convenience
    df = pd.DataFrame(dataset)

    # count occurrences of intents
    true_class_counts = Counter(df.true_label)
    pred_class_counts = Counter(df.pred_label)

    # transform strings into ids
    df['true'] = df.true_label.map(classname2id.get)
    df['pred'] = df.pred_label.map(classname2id.get)

    fielddate = datetime.date.today().isoformat()

    metrics = [
        {'value': accuracy_score(df.true, df.pred),
         'metric': 'accuracy',
         'average': 'not_applicable',
         'intent': '_total_',
         'fielddate': fielddate},
        # F1
        {'value': f1_score(df.true, df.pred, average='micro'),
         'metric': 'f1',
         'average': 'micro',
         'intent': '_total_',
         'fielddate': fielddate},
        {'value': f1_score(df.true, df.pred, average='macro'),
         'metric': 'f1',
         'average': 'macro',
         'intent': '_total_',
         'fielddate': fielddate},
        {'value': f1_score(df.true, df.pred, average='weighted'),
         'metric': 'f1',
         'average': 'weighted',
         'intent': '_total_',
         'fielddate': fielddate},
        # Precision
        {'value': precision_score(df.true, df.pred, average='micro'),
         'metric': 'precision',
         'average': 'micro',
         'intent': '_total_',
         'fielddate': fielddate},
        {'value': precision_score(df.true, df.pred, average='macro'),
         'metric': 'precision',
         'average': 'macro',
         'intent': '_total_',
         'fielddate': fielddate},
        {'value': precision_score(df.true, df.pred, average='weighted'),
         'metric': 'precision',
         'average': 'weighted',
         'intent': '_total_',
         'fielddate': fielddate},
        # recall
        {'value': recall_score(df.true, df.pred, average='micro'),
         'metric': 'recall',
         'average': 'micro',
         'intent': '_total_',
         'fielddate': fielddate},
        {'value': recall_score(df.true, df.pred, average='macro'),
         'metric': 'recall',
         'average': 'macro',
         'intent': '_total_',
         'fielddate': fielddate},
        {'value': recall_score(df.true, df.pred, average='weighted'),
         'metric': 'recall',
         'average': 'weighted',
         'intent': '_total_',
         'fielddate': fielddate}
    ]

    for i in id2classname:
        # for each intent i we score a binary classification [intent i] - [not intent i]
        true_i = (df.true == i).astype(int)
        pred_i = (df.pred == i).astype(int)

        metrics.append(
                {'value': accuracy_score(true_i, pred_i),
                 'metric': 'accuracy',
                 'intent': id2classname[i],
                 'average': 'not_applicable',
                 'fielddate': fielddate})
        metrics.append(
                {'value': precision_score(true_i, pred_i),
                 'metric': 'precision',
                 'average': 'binary',
                 'intent': id2classname[i],
                 'fielddate': fielddate})
        metrics.append(
                {'value': recall_score(true_i, pred_i),
                 'metric': 'recall',
                 'average': 'binary',
                 'intent': id2classname[i],
                 'fielddate': fielddate})
        metrics.append(
                {'value': f1_score(true_i, pred_i),
                 'metric': 'f1',
                 'average': 'binary',
                 'intent': id2classname[i],
                 'fielddate': fielddate})

    return [{'counters': {'fielddate': fielddate,
                          'true_class_counts': true_class_counts,
                          'pred_class_counts': pred_class_counts},
             'metrics': metrics}]


if __name__ == '__main__':
    call_as_operation(main, input_spec={
        'dataset': {'link_name': 'mapped_dataset', 'parser': 'json'}
    })
