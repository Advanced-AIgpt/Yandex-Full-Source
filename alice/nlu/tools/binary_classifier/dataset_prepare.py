from alice.nlu.tools.binary_classifier.dataset import Dataset
from alice.nlu.tools.binary_classifier.dataset_loader import INPUT_DATASET_FORMATS, OUTPUT_DATASET_FORMATS


# ==== Prepare complex datasets ====

def _get_input_dataset_keys(targets):
    return {target + '_' + fmt for target in targets for fmt in INPUT_DATASET_FORMATS}

GRANET_SPECIFIC_CONFIG_KEYS = {
    'positive', 'negative', 'ignore', 'base', 'negative_from_base_ratio', 'negative_from_base_count'
}
COMPLEX_DATASET_CONFIG_KEYS = {'name', 'disable', 'stub_storage', 'remove_duplicated', 'select', 'direct'}
DIRECT_DATASET_CONFIG_KEYS = _get_input_dataset_keys({'positive', 'negative'})
SELECTION_DATASET_CONFIG_KEYS = (
    _get_input_dataset_keys({'from'}) |
    {'by_column', 'by_text', 'negative_from_unknown_ratio', 'positive_from_unknown_ratio'}
)
SELECT_BY_COLUMN_CONFIG_KEYS = {'column', 'match_suffix'}

ALL_DATASET_CONFIG_KEYS = (
    GRANET_SPECIFIC_CONFIG_KEYS |
    COMPLEX_DATASET_CONFIG_KEYS |
    DIRECT_DATASET_CONFIG_KEYS |
    SELECTION_DATASET_CONFIG_KEYS |
    SELECT_BY_COLUMN_CONFIG_KEYS)


def _check_prohibited_keys(config, keys):
    for key in keys:
        if key in config:
            raise ValueError('Invalid dataset config: unexpected key "%s".' % key)


def _calc_complex_dataset_stat(dataset):
    targets = ['positive', 'negative']
    return {target: sum(len(info['indexes']) for info in dataset[target]) for target in targets}


def _print_key_value_stat(caption, stat):
    print('%-30s' % (caption + ':'), ', '.join(['%s: %d' % (k, v) for k, v in stat.items()]))


def _print_complex_dataset_stat(caption, dataset):
    _print_key_value_stat(caption, _calc_complex_dataset_stat(dataset))


def _print_selection_stat(caption, selection):
    _print_key_value_stat(caption, {target: len(indexes) for target, indexes in selection.items()})


def _create_initial_selection(dataset):
    return {
        'positive': [],
        'negative': [],
        'ignore': [],
        'unknown': list(range(dataset.row_count())),
    }


def _select_from_dataset_by_text(config, dataset_loader, dataset, selection):
    for target in ['ignore', 'negative', 'positive']:
        target_texts = set()
        for target_dataset in dataset_loader.load_from_config(config, target, None):
            target_texts |= {target_dataset.get_text(i) for i in range(target_dataset.row_count())}
        if not target_texts:
            continue
        target_selection = selection[target]
        new_unknown = []
        for i in selection['unknown']:
            text = dataset.get_text(i)
            if text in target_texts:
                target_selection.append(i)
            else:
                new_unknown.append(i)
        selection['unknown'] = new_unknown
    _print_selection_stat('    After select by text', selection)


def _select_from_dataset_by_column(config, dataset, selection):
    column = dataset.dataset.columns[config['column']]
    if config.get('match_suffix', False):
        match = (lambda label, suffixes: any(label.endswith(suffix) for suffix in suffixes))
    else:
        match = (lambda label, labels: label in labels)
    for target in ['ignore', 'negative', 'positive']:
        labels = config.get(target + '_value', [])
        if isinstance(labels, str):
            labels = [labels]
        if not labels:
            continue
        target_selection = selection[target]
        new_unknown = []
        for i in selection['unknown']:
            label = dataset.dataset.rows[i][column]
            if match(label, labels):
                target_selection.append(i)
            else:
                new_unknown.append(i)
        selection['unknown'] = new_unknown
    _print_selection_stat('    After select by column', selection)


def _select_from_dataset_by_ratio(config, selection):
    for target in ['ignore', 'negative', 'positive']:
        ratio = config.get(target + '_from_unknown_ratio', 0)
        if ratio == 0:
            continue
        target_selection = selection[target]
        new_unknown = []
        for i in selection['unknown']:
            grid = i * ratio
            if (grid - int(grid)) < ratio:
                target_selection.append(i)
            else:
                new_unknown.append(i)
        selection['unknown'] = new_unknown
    _print_selection_stat('    After select from unknown', selection)


def _prepare_selection_dataset(config, dataset_loader, stub_storage_path, complex_dataset):
    _check_prohibited_keys(config, ALL_DATASET_CONFIG_KEYS - SELECTION_DATASET_CONFIG_KEYS)

    from_datasets = dataset_loader.load_from_config(config, 'from', stub_storage_path)
    if not from_datasets:
        raise ValueError('Invalid dataset config: base dataset of selection is not defined (key "from_tsv").')
    if len(from_datasets) > 1:
        raise ValueError('Invalid dataset config: more than one base dataset of selection.')
    dataset = from_datasets[0]

    selection = _create_initial_selection(dataset)

    if 'by_text' in config:
        _select_from_dataset_by_text(config['by_text'], dataset_loader, dataset, selection)

    if 'by_column' in config:
        _select_from_dataset_by_column(config['by_column'], dataset, selection)

    _select_from_dataset_by_ratio(config, selection)

    for target in ['positive', 'negative']:
        complex_dataset[target].append({
            'dataset': dataset,
            'indexes': selection[target],
        })


def _prepare_direct_dataset(config, dataset_loader, stub_storage_path, complex_dataset):
    _check_prohibited_keys(config, ALL_DATASET_CONFIG_KEYS - DIRECT_DATASET_CONFIG_KEYS)

    for target in ['positive', 'negative']:
        for dataset in dataset_loader.load_from_config(config, target, stub_storage_path):
            complex_dataset[target].append({
                'dataset': dataset,
                'indexes': list(range(dataset.row_count())),
            })


def _get_texts_of_selections(selections):
    texts = set()
    for selection in selections:
        dataset = selection['dataset']
        texts |= {dataset.get_text(i) for i in selection['indexes']}
    return texts


def _remove_conflicted_samples(config, complex_dataset):
    should_remove_from = {
        'negative': config.get('remove_conflicted_from_negative', False) or config.get('remove_conflicted', False),
        'positive': config.get('remove_conflicted_from_positive', False) or config.get('remove_conflicted', False),
    }
    if not should_remove_from['negative'] and not should_remove_from['positive']:
        return
    conflicted_texts = (
        _get_texts_of_selections(complex_dataset['positive']) &
        _get_texts_of_selections(complex_dataset['negative'])
    )
    for target in ['positive', 'negative']:
        if not should_remove_from[target]:
            continue
        for selection in complex_dataset[target]:
            dataset = selection['dataset']
            selection['indexes'] = [i for i in selection['indexes'] if dataset.get_text(i) not in conflicted_texts]
    _print_complex_dataset_stat('    After remove conflicted', complex_dataset)


def _remove_duplicated_samples(config, complex_dataset):
    if not config.get('remove_duplicated', False):
        return
    for target in ['positive', 'negative']:
        texts = set()
        for info in complex_dataset[target]:
            dataset = info['dataset']
            new_indexes = []
            for i in info['indexes']:
                text = dataset.get_text(i)
                if text in texts:
                    continue
                texts.add(text)
                new_indexes.append(i)
            info['indexes'] = new_indexes
    _print_complex_dataset_stat('    After remove duplicated', complex_dataset)


def _prepare_complex_dataset(parent_config, config, dataset_loader):
    _check_prohibited_keys(config, ALL_DATASET_CONFIG_KEYS - COMPLEX_DATASET_CONFIG_KEYS)

    complex_dataset = {
        'positive': [],
        'negative': [],
        'config': config,
    }
    stub_storage_path = config.get('stub_storage', parent_config.get('stub_storage'))

    if 'select' in config:
        _prepare_selection_dataset(config['select'], dataset_loader, stub_storage_path, complex_dataset)
    if 'direct' in config:
        _prepare_direct_dataset(config['direct'], dataset_loader, stub_storage_path, complex_dataset)
    if 'select' not in config and 'direct' not in config:
        raise ValueError('Invalid dataset config: unknown type of dataset.')

    _remove_conflicted_samples(config, complex_dataset)
    _remove_duplicated_samples(config, complex_dataset)

    _print_complex_dataset_stat('    Selected', complex_dataset)
    print('    Columns: %s.' % ', '.join(_get_complex_datasets_common_columns([complex_dataset])))
    return complex_dataset


def prepare_complex_datasets(config, dataset_loader):
    print('Prepare datsets:')
    result = {}
    for dataset_config in config.get('datasets', []):
        if dataset_config.get('disable', False):
            continue
        name = dataset_config['name']
        print('  Prepare dataset "%s":' % name)
        result[name] = _prepare_complex_dataset(config, dataset_config, dataset_loader)
    return result


# ==== Print statistics ====

def print_complex_dataset_collection_stat(datasets):
    print('Datasets:')
    name_max_len = max(15, max(len(name) for name in datasets))
    print('  %s      Pos      Neg' % 'Dataset'.ljust(name_max_len))
    for name, dataset in datasets.items():
        stat = _calc_complex_dataset_stat(dataset)
        print('  %s %8d %8d' % (name.ljust(name_max_len), stat['positive'], stat['negative']))


# ==== Save datasets ====

def _get_complex_datasets_common_columns(complex_datasets):
    common = None
    for complex_dataset in complex_datasets:
        for target in ['positive', 'negative']:
            for selection in complex_dataset[target]:
                header = selection['dataset'].dataset.header
                if common is None:
                    common = header
                    continue
                common = [name for name in common if name in header]
    if common is None:
        common = []
    return common


def _create_preprocessed_dataset(complex_datasets, columns, embedding_column, label_column, dst_target):
    dst_dataset = Dataset(header=columns)
    for complex_dataset in complex_datasets:
        for src_target, label in [('positive', 1), ('negative', 0)]:
            if dst_target != 'labeled' and src_target != dst_target:
                continue
            for selection in complex_dataset[src_target]:
                src_dataset = selection['dataset'].dataset
                for row_index, embedding in zip(selection['indexes'], selection['raw_embeddings']):
                    src_row = src_dataset.rows[row_index]
                    dst_row = []
                    for column in columns:
                        src_column_index = src_dataset.columns.get(column)
                        if src_column_index is not None:
                            dst_row.append(src_row[src_column_index])
                        elif column == embedding_column:
                            dst_row.append(embedding)
                        elif column == label_column:
                            dst_row.append(label)
                        elif column == 'weight':
                            dst_row.append(1)
                        else:
                            dst_row.append(None)
                    dst_dataset.rows.append(dst_row)
    return dst_dataset


def _save_preprocessed_dataset(config, embedding_column, target, fmt, datasets):
    path = config.get(target + '_' + fmt)
    if not path:
        return
    print('  Save "%s":' % path)

    dataset_names = config.get('datasets', [])
    complex_datasets = [datasets[name] for name in dataset_names]

    label_column = config.get('label_column', 'label')

    columns = config.get('columns')
    if columns is None:
        columns = _get_complex_datasets_common_columns(complex_datasets)
        if embedding_column not in columns:
            columns.append(embedding_column)
        if target == 'labeled' and label_column not in columns:
            columns.insert(0, label_column)

    out_dataset = _create_preprocessed_dataset(complex_datasets, columns, embedding_column, label_column, target)
    print('    Rows: %d. Columns: %s.' % (len(out_dataset.rows), ', '.join(out_dataset.header)))
    out_dataset.save(path, fmt)


def save_preprocessed_datasets(config, datasets):
    output_configs = config.get('save_preprocessed_datasets', [])
    if not output_configs:
        return
    print('Save preprocessed datasets:')
    embedding_column = 'embedding_' + config['model']['embedding']
    for output_config in output_configs:
        if output_config.get('disable', False):
            continue
        for target in ['positive', 'negative', 'labeled']:
            for fmt in OUTPUT_DATASET_FORMATS:
                _save_preprocessed_dataset(output_config, embedding_column, target, fmt, datasets)
