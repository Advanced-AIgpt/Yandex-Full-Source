def get_training_config(v):
    d = v['beggins']
    d['model_name'] = v['model_name']
    manifest = v['manifest']
    d['classification_task_question'] = manifest['classification_task_question']
    d['classification_project_instructions'] = manifest['classification_project_instructions']
    d['positive_honeypots'] = manifest['positive_honeypots']
    d['negative_honeypots'] = manifest['negative_honeypots']
    return d


def get_toloka_parameters_from_config(v):
    manifest = v['manifest']
    return {
        'classification_task_question': manifest['classification_task_question'],
        'classification_project_instructions': manifest['classification_project_instructions'],
        'positive_honeypots': manifest['positive_honeypots'],
        'negative_honeypots': manifest['negative_honeypots'],
    }


def join_single_intent_metrics(v, w, x):
    pre_accept = v
    true_accept = w
    METRICS_FOR_DIFF = ['precision', 'recall', 'f1', 'roc_auc', 'accuracy']
    diff = {key: pre_accept['metrics'][key] - true_accept['metrics'][key] for key in METRICS_FOR_DIFF}
    return {
        'pre_accept': pre_accept,
        'true_accept': true_accept,
        'pre_vs_true_accept_metrics_diff': diff,
    }


def join_metrics_and_model_name(v, w, x):
    config = v
    metrics = w
    matches = x
    result = {}
    result['model_name'] = config['model_name']
    result.update(metrics)
    result['year_logs_matches'] = matches
    return result


def aggregate_metrics(v):
    import statistics

    METRICS = ['precision', 'recall', 'f1', 'roc_auc', 'accuracy']

    def get_by_key_path(d, path):
        for key in path.split('.'):
            if not isinstance(d, dict):
                return None
            d = d.get(key)
        return d

    def get_series(intents, key_path):
        return [get_by_key_path(intent, key_path) for intent in intents]

    def calculate_metrics_avg(intents, key_path):
        return {metric: statistics.mean(get_series(intents, f'{key_path}.{metric}')) for metric in METRICS}

    def calculate_metrics_std(intents, key_path):
        return {metric: statistics.pstdev(get_series(intents, f'{key_path}.{metric}')) for metric in METRICS}

    avg = {
        'pre_accept_metrics': calculate_metrics_avg(v, 'pre_accept.metrics'),
        'true_accept_metrics': calculate_metrics_avg(v, 'true_accept.metrics'),
        'pre_vs_true_accept_metrics_diff': calculate_metrics_avg(v, 'pre_vs_true_accept_metrics_diff'),
        'year_logs_matches': {
            'weighted_ratio': statistics.mean(get_series(v, 'year_logs_matches.weighted.ratio')),
            'unique_ratio': statistics.mean(get_series(v, 'year_logs_matches.unique.ratio')),
        }
    }
    std = {
        'pre_accept_metrics': calculate_metrics_std(v, 'pre_accept.metrics'),
        'true_accept_metrics': calculate_metrics_std(v, 'true_accept.metrics'),
    }

    return {
        'mf1': avg['true_accept_metrics']['f1'],
        'std_f1': std['true_accept_metrics']['f1'],
        'dmf1': avg['pre_vs_true_accept_metrics_diff']['f1'],
        'avg': avg,
        'std': std,
    }


def format_metrics_table(v):
    import statistics

    def shorten_model_name(name):
        return name.replace('alice.apps_fixlist.', '').replace('.beggins', '')

    def format_row(intent_info, accept_type):
        model_name = shorten_model_name(intent_info['model_name'])
        m = intent_info[accept_type]['metrics']
        fp = m['false_positive']
        fn = m['false_negative']
        tp = m['true_positive']
        tn = m['true_negative']
        gp = tp + fn
        gn = tn + fp
        rnd_match = intent_info['year_logs_matches']['weighted']['ratio']
        return f'  {model_name:16} {m["f1"]:.3f}  {m["precision"]:.3f}  {m["recall"]:.3f}  {m["roc_auc"]:.3f}  [{fn:4} {tp:4} {gp:4}]  [{fp:4} {tn:4} {gn:4}]  {rnd_match:.6f}'

    def format_aggregation_row(launch_info, accept_type, aggregation_name, aggregation_func):
        def calc(metric):
            return aggregation_func([item[accept_type]['metrics'][metric] for item in launch_info])
        rnd_match = aggregation_func([item['year_logs_matches']['weighted']['ratio'] for item in launch_info])
        return f'  {aggregation_name:16} {calc("f1"):.3f}  {calc("precision"):.3f}  {calc("recall"):.3f}  {calc("roc_auc"):.3f}  {"":34}  {rnd_match:.6f}'

    out = []
    for accept_type in ['pre_accept', 'true_accept']:
        out.append(accept_type)
        out.append('  Model              F1  Precis Recall RocAuc  [  FN + TP = GP]  [  FP + TN = GN]  RndMatch')
        for intent_info in v:
            out.append(format_row(intent_info, accept_type))
        out.append(format_aggregation_row(v, accept_type, 'Std', statistics.pstdev))
        out.append(format_aggregation_row(v, accept_type, 'Average', statistics.mean))
    return out
