# coding: utf-8

from alice.nlu.tools.binary_classifier.metrics_counter import MetricsCounter
from alice.nlu.tools.binary_classifier.utils import save_tsv_simple, save_lines


def _prepare_outputs(config):
    output_list = []
    output_dict = {}
    for prefix in ['true', 'false', 'result', 'target']:
        for suffix in ['positive', 'negative']:
            path = config.get('%s_%s_tsv' % (prefix, suffix))
            if not path:
                continue
            is_positive = suffix == 'positive'
            if prefix == 'true':
                keys = [(is_positive, is_positive)]
            elif prefix == 'false':
                keys = [(not is_positive, is_positive)]
            elif prefix == 'result':
                keys = [(True, is_positive), (False, is_positive)]
            elif prefix == 'target':
                keys = [(is_positive, True), (is_positive, False)]
            output = {
                'path': path,
                'rows': [],
            }
            output_list.append(output)
            for key in keys:
                output_dict.setdefault(key, []).append(output)
    return output_list, output_dict


def _make_output_row(dataset, index, prob, columns):
    row = []
    for column in columns:
        if column == 'probability':
            row.append('%.3f' % prob)
        elif column in dataset.dataset.columns:
            row.append(dataset.dataset.rows[index][dataset.dataset.columns[column]])
        else:
            row.append('')
    return row


def _test_model_on_dataset(model, threshold, config, datasets):
    dataset_name = config['dataset']
    columns = config.get('columns', ['text'])
    output_list, output_dict = _prepare_outputs(config)
    metrics = MetricsCounter()

    for target in ['positive', 'negative']:
        is_target_positive = target == 'positive'

        for selection in datasets[dataset_name][target]:
            dataset = selection['dataset']
            indexes = selection['indexes']
            if not indexes:
                continue

            probs = model.predict(selection)

            for prob, index in list(zip(probs, indexes)):
                is_result_positive = prob >= threshold
                metrics.add_single_prediction(target=is_target_positive, pred=is_result_positive)
                for output in output_dict.get((is_target_positive, is_result_positive), []):
                    output['rows'].append(_make_output_row(dataset, index, prob, columns))

    for output in output_list:
        save_tsv_simple(columns, output['rows'], output['path'])
    return metrics


def test_model(model, config, datasets):
    test_config = config.get('test', {})
    if not test_config:
        return
    summary_lines = []
    def print_summary(line):
        print(line)
        summary_lines.append(line)
    print_summary('Test:')
    case_configs = test_config.get('cases', [])
    name_max_len = max(15, max(len(t['dataset']) for t in case_configs))
    print_summary('  %s  Thr   Prec  Rec  F1    Excess / TrgtNeg     Lost / TrgtPos' % 'Dataset'.ljust(name_max_len))
    for case_config in case_configs:
        threshold = case_config.get('threshold', test_config.get('threshold', 0.5))
        metrics = _test_model_on_dataset(model, threshold, case_config, datasets)
        print_summary('  %s  %-5.3g %3.f%% %3.f%% %3.f%% %8d /%8d %8d /%8d' % (
            case_config['dataset'].ljust(name_max_len),
            threshold,
            metrics.precision * 100,
            metrics.recall * 100,
            metrics.f1 * 100,
            metrics.fp,
            metrics.target_negative,
            metrics.fn,
            metrics.target_positive,
        ))
    summary_path = test_config.get('summary')
    if summary_path:
        save_lines(summary_lines, summary_path)
