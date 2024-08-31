import numpy as np
import click
import sys
import pandas as pd
import os
import json
import re
import warnings

from collections import defaultdict, Counter, OrderedDict
from sklearn.metrics import precision_recall_fscore_support, accuracy_score, f1_score
from sklearn.preprocessing import LabelEncoder
from functools import partial
from sklearn.metrics import classification_report

from vins_core.utils.intent_renamer import IntentRenamer


warnings.filterwarnings("ignore")


def _get_weights(weights, y):
    if weights is None:
        weights = np.ones_like(y, dtype=np.float32)

    return weights


def _accuracy_optimizer(all_weights, minf_all_winners, y, significant_y, inf_winners, minf_winners, scores,
                        weights, other_label):
    minf_loss = (significant_y != minf_winners) * weights
    inf_loss = (significant_y != inf_winners) * weights

    scores = np.insert(scores, 0, -np.inf)
    minf_loss = np.append(minf_loss, 0)
    inf_loss = np.insert(inf_loss, 0, 0)

    inf_loss = np.cumsum(inf_loss)
    minf_loss = np.cumsum(minf_loss[::-1])[::-1]
    scores, minf_index, counts = np.unique(scores, return_index=True, return_counts=True)
    inf_index = minf_index + counts - 1

    inf_loss = inf_loss[inf_index]
    minf_loss = minf_loss[minf_index]
    scores[-1] = np.inf

    loss = inf_loss + minf_loss

    return scores[np.argmin(loss)]


def _safe_div(nom, denom):
    return float(nom) / denom if denom != 0 else 0.


def _update_intent_stats(intents, pred_intent, true_intent, delta):
    tp = true_intent == pred_intent

    if tp:
        intents[pred_intent]['tp'] += delta
    else:
        intents[pred_intent]['fp'] += delta
        intents[true_intent]['fn'] += delta

    return tp


def _update_intents_f_scores(intents, intents_to_update, other_label, beta):
    f_score_sum = 0

    for intent in intents_to_update:
        if intents[intent]['support'] > 0 and intent != other_label:
            tp, fp, fn = [intents[intent][label] for label in ['tp', 'fp', 'fn']]
            prec = _safe_div(tp, tp + fp)
            rec = _safe_div(tp, tp + fn)
            beta2 = beta[intent] ** 2
            f_score = _safe_div((1 + beta2) * prec * rec, beta2 * prec + rec)

            f_score_sum += f_score - intents[intent]['f_score']
            intents[intent]['f_score'] = f_score

    return f_score_sum


def _macro_f1_optimizer(all_weights, minf_all_winners, y, significant_y, inf_winners, minf_winners, scores,
                        weights, other_label, beta):
    intents = defaultdict(lambda: defaultdict(int))
    for weight, pred_intent, true_intent in zip(all_weights, minf_all_winners, y):
        _update_intent_stats(intents, pred_intent, true_intent, weight)
        intents[true_intent]['support'] += 1

    f_score_sum = _update_intents_f_scores(intents, intents.keys(), other_label, beta)

    different_scores = [-np.inf]
    f_scores = [f_score_sum]

    for i, (from_intent, to_intent, true_intent, score, weight) in enumerate(zip(minf_winners, inf_winners,
                                                                                 significant_y, scores, weights)):
        _update_intent_stats(intents, from_intent, true_intent, -weight)
        _update_intent_stats(intents, to_intent, true_intent, weight)
        f_score_sum += _update_intents_f_scores(intents, [from_intent, to_intent, true_intent], other_label, beta)

        if i + 1 == len(scores) or scores[i + 1] != score:
            different_scores.append(score)
            f_scores.append(f_score_sum)

    return different_scores[np.argmax(f_scores)]


def _get_labels(y_true, other_label):
    return list(set(y_true) - {other_label})


def _get_macro_f1_score(y_true, y_pred, other_label, weights, beta=None):
    labels = _get_labels(y_true, other_label)

    beta2 = (beta[labels] if beta is not None else 1.) ** 2

    precision, recall, _, _ = precision_recall_fscore_support(y_true, y_pred, labels=labels, sample_weight=weights)

    return np.mean(np.nan_to_num(((1 + beta2) * precision * recall / (beta2 * precision + recall))))


def _get_accuracy_score(y_true, y_pred, other_label, weights):
    return accuracy_score(y_true, y_pred, sample_weight=weights)


def _fit_threshold(x, y, thresholds, index, other_label, pred_renames, optimizer, weights, fit_delta):
    weights = _get_weights(weights, y)

    x = x.copy()
    thresholds_copy = thresholds.copy()
    thresholds_copy[index] = -np.inf
    x[x <= thresholds_copy] = -np.inf

    minf_all_winners = x.argmax(axis=1)
    significant_mask = np.array([item in index for item in minf_all_winners])

    if not np.any(significant_mask):
        return thresholds

    significant_x = x[significant_mask]
    significant_y = y[significant_mask]
    significant_weights = weights[significant_mask]
    significant_winners = minf_all_winners[significant_mask]

    temp = np.arange(significant_x.shape[0])
    scores = significant_x[temp, significant_winners]
    if fit_delta:
        scores -= thresholds[significant_winners]

    minf_winners = pred_renames[significant_winners]

    significant_x[:, index] = -np.inf
    inf_winners = significant_x.argmax(axis=1)
    inf_winners[significant_x[temp, inf_winners] == -np.inf] = other_label
    inf_winners = pred_renames[inf_winners]

    scores_sort = np.argsort(scores)
    for item in [scores, inf_winners, minf_winners, significant_y, significant_weights]:
        item[:] = item[scores_sort]

    result = optimizer(weights, pred_renames[minf_all_winners], y, significant_y, inf_winners, minf_winners,
                       scores, significant_weights, pred_renames[other_label])

    thresholds_copy = thresholds.copy()
    if fit_delta:
        thresholds_copy[index] += result
    else:
        thresholds_copy[index] = result

    return thresholds_copy


def _get_thresholds_score(x, y, thresholds, other_label, pred_renames, get_score, weights=None):
    weights = _get_weights(weights, y)
    x = x.copy()

    temp = np.arange(x.shape[0])
    x[x <= thresholds] = -np.inf
    y_pred = np.argmax(x, axis=1)

    y_pred[x[temp, y_pred] == -np.inf] = other_label

    return get_score(y, pred_renames[y_pred], pred_renames[other_label], weights)


def _fit_thresholds(x, y, custom_thresholds, other_label, pred_renames, get_score, optimizer, init_thresholds,
                    weights=None, num_epochs=1, fit_delta=False):
    num_classes = x.shape[1]

    thresholds = init_thresholds
    all_sets = [[i] for i in custom_thresholds]
    default_threshold = [t for t in np.arange(num_classes) if t not in custom_thresholds]
    default_threshold_fit = len(default_threshold) > 0
    if default_threshold_fit:
        all_sets.append(default_threshold)

    score_trace = []
    for i in xrange(num_epochs):
        np.random.shuffle(all_sets)

        for j, s in enumerate(all_sets):
            thresholds = _fit_threshold(x, y, thresholds, s, other_label, pred_renames, optimizer, weights, fit_delta)
            score = _get_thresholds_score(x, y, thresholds, other_label, pred_renames, get_score, weights)
            if len(score_trace) > 0:
                assert score_trace[-1] <= score, (score_trace[-1], score)

            print 'Epoch {}, iteration {}, score {:.5f}'.format(i, j, score)

    return {
        'thresholds': thresholds,
        'default_threshold': thresholds[default_threshold[0]] if default_threshold_fit else None
    }


def _get_thresholds(threshold_dict, intent_names):
    thresholds = np.full((len(intent_names),), threshold_dict.get('default', 0), dtype=np.float32)

    for key, value in threshold_dict.iteritems():
        if key != 'default':
            thresholds[intent_names.index(key)] = value

    return thresholds


@click.group()
def main():
    pass


@main.command('make_custom_intents', short_help='Create *.nlu files and *.mlconfig.json for model fit')
@click.option('--classifier', type=str, help='Classifier to prepare intents file for')
@click.option('--markup-file', type=click.Path(), help='Toloka markup tsv file')
@click.option('--output-dir', type=click.Path(), help='Output directory to store "*.nlu" '
                                                      'and "*.mlconfig.json" files in')
@click.option('--text-col', help='text column name', default='text')
@click.option('--intent-col', help='intent column name', default='intent')
def make_custom_intents(classifier, markup_file, output_dir, text_col, intent_col):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    intents_path = os.path.join(output_dir, 'intents')
    if not os.path.exists(intents_path):
        os.makedirs(intents_path)

    df = pd.read_csv(markup_file, sep='\t')
    df = df[~df.text.str.contains('\n')]

    groups = df.groupby(intent_col).groups
    intents = []

    for intent_name, index in groups.iteritems():
        path = os.path.join('intents', '{}.nlu'.format(intent_name))

        with open(os.path.join(output_dir, path), 'w') as outfile:
            outfile.write('\n'.join(df.loc[index, text_col]))

        intent_entry = {
            "intent": intent_name,
            "trainable_classifiers": [classifier],
            "nlu": [{
                "source": "file",
                "path": path,
                "unique": False
            }]
        }

        intents.append(intent_entry)

    result = {
        'name': 'threshold_fit',
        'intents': intents
    }

    with open(os.path.join(output_dir, 'threshold_fit.mlconfig.json'), 'w') as outfile:
        json.dump(result, outfile, indent=2)


def _parse_initial(intent_names, pred_renames, fname, num_classes):
    parse_result = {}

    with open(fname, 'r') as infile:
        for line in infile.read().splitlines():
            intent, value = line.split(' ')
            parse_result[intent] = float(value)

    result = np.full((num_classes,), parse_result.get('default', 1.), dtype=np.float32)
    intents = []
    for intent, value in parse_result.iteritems():
        if intent != 'default':
            if intent not in intent_names:
                raise ValueError('There is an intent "{}" in {} file that doesn\'t exist'.format(intent, fname))

            index = intent_names.index(intent)
            result[pred_renames[index]] = value
            intents.append(index)

    return result, intents


def _prepare_knn_output(input_dir, rename, other, filter_utterances=None):
    print "Preparing knn output data..."

    Xs, ys = [], []
    renamer = IntentRenamer(rename)
    intent_names = None

    for fname in os.listdir(input_dir):
        intent_name = renamer('.'.join(fname.split('.')[2:-1]), IntentRenamer.By.TRUE_INTENT)

        df = pd.read_csv(os.path.join(input_dir, fname), sep='\t', encoding='utf-8', index_col='index')

        if filter_utterances is not None:
            df = df[df.index.str.match(filter_utterances)]

        if intent_names is None:
            regex = re.compile('.*threshold_fit.*')
            intent_names = [name for name in df.columns if regex.match(name) is None]

        Xs.append(df[intent_names].values)
        ys.append(intent_name)

    intent_names_normalized = [renamer(name, IntentRenamer.By.PRED_INTENT) for name in intent_names]

    le = LabelEncoder().fit(intent_names_normalized + ys)
    pred_renames = le.transform(intent_names_normalized)
    ys = np.concatenate([np.full((x.shape[0],), le.transform([y])[0], dtype=np.int32) for x, y in zip(Xs, ys)])
    Xs = np.concatenate(Xs)

    other_regex = re.compile(other)
    other_ind = [name for name in intent_names if other_regex.match(name)]
    if len(other_ind) != 1:
        raise ValueError('Only 1 intent must fit other regex. Regex: "{}", '
                         'intents: "{}'.format(other, ';'.join(other_ind)))
    other_ind = intent_names.index(other_ind[0])

    y_pred = pred_renames[Xs.argmax(axis=1)]
    labels = _get_labels(ys, other_ind)
    print 'Accuracy of plain KNNModel: {:.5f}'.format(accuracy_score(ys, y_pred))
    print 'Macro F1 Score of plain KNNModel: {:.5f}'.format(f1_score(ys, y_pred, labels=labels, average='macro'))
    train_samples_num = np.isclose(Xs.max(axis=1), 1., atol=1e-7).sum()
    print 'Number of samples in toloka set that encounter in train set: {}/{}'.format(
        train_samples_num, Xs.shape[0])
    print 'Report:\n{}'.format(classification_report(ys, y_pred, labels, target_names=le.inverse_transform(labels)))

    return Xs, ys, intent_names, pred_renames, le, other_ind


@main.command('threshold_fit', short_help='Create *.nlu files and *.mlconfig.json for model fit')
@click.option('--input-dir', type=click.Path(), help='Path to app_analyzer cross_classify outputs')
@click.option('--rename', default='{}', help='JSON string or filepath with regexp-to-new-intent-name mappings')
@click.option('--optimize-for', type=click.Choice(['accuracy', 'macro_f1_score']))
@click.option('--custom-thresholds', type=click.Path(), help='Path to file with intent names. Custom threshold will be '
                                                             'fitted for every intent in the file and for the other '
                                                             'intents one common threshold will be fitted.')
@click.option('--fbeta', type=click.Path(), help='Path to file where custom betas can be set for every intent '
                                                 '(this is useful if you optimze for macro f1 score). On every line of '
                                                 'file you should write "$intent_name $beta". You can also set default '
                                                 'beta the following way: "default $beta"')
@click.option('--num-epochs', type=int, help='Epoch number')
@click.option('--other', type=str, help='Regex for other label', default='.*other')
@click.option('--lower-bound', type=int,
              help='Minimum number of samples to fit custom threshold for intent', default=0)
@click.option('--init-loc', type=float,
              help='loc of normal distribution (intitialization of thresholds)', default=0.8)
@click.option('--init-std', type=float,
              help='std of normal distribution (intitialization of thresholds)', default=0.1)
@click.option('--init-custom', type=click.Path(),
              help='Path to file with custom initial value of thresholds. Format of line: "$intent_name $threshold"')
@click.option('--fit-delta', is_flag=True,
              help='Let initial value of thresholds be a, then algorithm will fit some '
                   '\delta and final thresholds will be a + \delta (taking into account custom thresholds)')
@click.option('--output-file', type=click.Path(),
              help='output file to write thresholds to', default='thresholds.json')
def threshold_fit(input_dir, rename, optimize_for, custom_thresholds, fbeta, num_epochs, other, lower_bound, init_loc,
                  init_std, init_custom, fit_delta, output_file):
    Xs, ys, intent_names, pred_renames, le, other_ind = _prepare_knn_output(input_dir, rename, other)

    if custom_thresholds is not None:
        with open(custom_thresholds, 'r') as infile:
            custom_intents = [intent_names.index(line) for line in infile.read().splitlines()]
    else:
        custom_intents = []

    output_intents = custom_intents

    intent_counts = Counter(ys)
    custom_thresholds_inds = [intent for intent in custom_intents
                              if intent_counts[pred_renames[intent]] >= lower_bound]

    if optimize_for == 'accuracy':
        get_score, optimizer = _get_accuracy_score, _accuracy_optimizer
    elif optimize_for == 'macro_f1_score':
        if fbeta is not None:
            betas, _ = _parse_initial(intent_names, pred_renames, fbeta, le.classes_.shape[0])
        else:
            betas = np.full(le.classes_.shape, 1., dtype=np.float32)

        get_score, optimizer = partial(_get_macro_f1_score, beta=betas), partial(_macro_f1_optimizer, beta=betas)
    else:
        raise ValueError('Unknown optimization objective {}'.format(optimize_for))

    num_classes = Xs.shape[1]
    if init_custom is not None:
        init_thresholds, intents = _parse_initial(intent_names, np.arange(len(intent_names)), init_custom, num_classes)

        if fit_delta:
            output_intents = set(intents + output_intents)
    else:
        init_thresholds = np.random.normal(loc=init_loc, scale=init_std, size=num_classes)

    thresholds = _fit_thresholds(Xs, ys, custom_thresholds_inds, other_ind, pred_renames, get_score, optimizer,
                                 init_thresholds, weights=None, num_epochs=num_epochs, fit_delta=fit_delta)

    print 'Thresholds fitted'
    print 'Accuracy: {}'.format(_get_thresholds_score(Xs, ys, thresholds['thresholds'], other_ind,
                                                      pred_renames, _get_accuracy_score, weights=None))
    print 'Macro F1 Score: {}'.format(_get_thresholds_score(Xs, ys, thresholds['thresholds'], other_ind,
                                                            pred_renames, _get_macro_f1_score, weights=None))

    result = {intent_names[i]: thresholds['thresholds'][i] for i in output_intents}
    result['default'] = thresholds['default_threshold']

    for key, value in result.iteritems():
        if value == np.inf:
            value = 1.
        elif value == -np.inf:
            value = -1.
        else:
            value = round(value, 3)

        result[key] = value

    with open(output_file, 'w') as outfile:
        json.dump(result, outfile, indent=4)

    print json.dumps(result, indent=4)


@main.command('get_knn_scores', short_help='Get metrics for intent prediction based on KNNModel only '
                                           'and with thresholds')
@click.option('--input-dir', type=click.Path(), help='Path to app_analyzer cross_classify outputs')
@click.option('--rename', default='{}', help='JSON string or filepath with regexp-to-new-intent-name mappings')
@click.option('--thresholds-file', type=click.Path(), help='Path to file with thresholds in format util '
                                                           'threshold_fit outputs', default='thresholds.json')
@click.option('--other', type=str, help='Regex for other label', default='.*other')
@click.option('--filter-utterances', type=str, help='Regex to filter test utterances')
def get_knn_scores(input_dir, rename, thresholds_file, other, filter_utterances):
    Xs, ys, intent_names, pred_renames, le, other_ind = _prepare_knn_output(
        input_dir, rename, other, filter_utterances)

    if os.path.exists(thresholds_file):
        with open(thresholds_file, 'r') as infile:
            thresholds_dict = json.load(infile)

        thresholds = _get_thresholds(thresholds_dict, intent_names)

        print 'Scores of KNNModel with thresholds'
        print 'Accuracy: {}'.format(_get_thresholds_score(Xs, ys, thresholds, other_ind,
                                                          pred_renames, _get_accuracy_score, weights=None))
        print 'Macro F1 Score: {}'.format(_get_thresholds_score(Xs, ys, thresholds, other_ind,
                                                                pred_renames, _get_macro_f1_score, weights=None))
    else:
        print 'Metrics of model with thresholds can\'t be calculated ' \
              'as {} file doesn\'t exist'.format(thresholds_file)


@main.command('apply_thresholds', short_help='Write thresholds in files configuration files')
@click.option('--intent-prefix', help='$app.$classifier prefix for intents you want to write in VinsProjectfile',
              default='personal_assistant.scenarios')
@click.option('--vinsfile', type=click.Path(), help='Vinsfile to write default threshold in',
              default='apps/personal_assistant/personal_assistant/config/Vinsfile.json')
@click.option('--vins-project-file', type=click.Path(), help='VinsProjectfile',
              default='apps/personal_assistant/personal_assistant/config/scenarios/VinsProjectfile.json')
@click.option('--thresholds-file', type=click.Path(), help='Path to file with thresholds in util '
                                                           'threshold_fit outputs',
              default='thresholds.json')
@click.option('--classifier-pos', type=int, help='Classifier position in cascade', default=6)
def apply_thresholds(intent_prefix, vinsfile, vins_project_file, thresholds_file, classifier_pos):
    with open(thresholds_file, 'r') as infile:
        thresholds_dict = json.load(infile)

    with open(vinsfile, 'r') as infile:
        vf = json.load(infile, object_pairs_hook=OrderedDict)

    vf['nlu']['fallback_thresholds'][classifier_pos] = thresholds_dict['default']

    with open(vinsfile, 'w') as outfile:
        json.dump(vf, outfile, indent=2)

    print 'Default threshold is written'
    with open(vins_project_file, 'r') as infile:
        vpf = json.load(infile, object_pairs_hook=OrderedDict)

    entry_index = {}
    for entry in vpf['intents']:
        if 'fallback_threshold' in entry:
            del entry['fallback_threshold']

        entry_index['{}.{}'.format(intent_prefix, entry['intent'])] = entry

    for intent, threshold in thresholds_dict.iteritems():
        if intent != 'default':
            entry = entry_index.get(intent)

            if entry is not None:
                print '{} is written'.format(intent)

                entry['fallback_threshold'] = threshold
            else:
                print '{} not found'.format(intent)

    with open(vins_project_file, 'w') as outfile:
        json.dump(vpf, outfile, indent=2)


if __name__ == '__main__':
    sys.exit(main())
