import numpy as np


# ==== Build input description ====

def _collect_feature_names(config, datasets):
    prefixes = config['model'].get('feature_prefixes', [])
    if not prefixes:
        return []
    print('  Collect feature names:')
    names = set()
    for dataset_name, complex_dataset in datasets.items():
        print('    Collect feature names for dataset "%s"' % dataset_name)
        for target in ['positive', 'negative']:
            for selection in complex_dataset[target]:
                dataset = selection['dataset']
                indexes = selection['indexes']
                for i in indexes:
                    for name, value in dataset.get_features(i).items():
                        names.add(name)
    names = [name for name in names if any(name.startswith(prefix) for prefix in prefixes)]
    names.sort()
    return names


def create_input_description(config, datasets):
    print('Create input description:')
    features = _collect_feature_names(config, datasets)
    print('  Number of selected features:', len(features))
    return {
        'sentence': {
            'vector_size': None,
            'embedding': config['model'].get('embedding', ''),
            'features': features,
        }
    }


# ==== Prepare embeddings ====

def _create_feature_vectors(dataset, indexes, feature_names):
    feature_map = {name: i for i, name in enumerate(feature_names)}
    feature_rows = []
    for row_index in indexes:
        feature_row = np.zeros(len(feature_names), dtype=np.float16)
        if feature_map:
            for name, value in dataset.get_features(row_index).items():
                feature_index = feature_map.get(name)
                if feature_index is None:
                    continue
                feature_row[feature_index] = value
        feature_rows.append(feature_row)
    return np.array(feature_rows)


def _create_sentence_vectors(input_description, selection):
    dataset = selection['dataset']
    indexes = selection['indexes']
    vectors = []

    embedding_name = input_description['sentence']['embedding']
    if embedding_name:
        full_name = 'embedding_' + embedding_name
        vectors.append(np.array([dataset.get_embedding(i, full_name) for i in indexes]))

    feature_names = input_description['sentence']['features']
    if feature_names:
        vectors.append(_create_feature_vectors(dataset, indexes, feature_names))

    return np.concatenate(vectors, axis=1)


def prepare_numpy_data(input_description, datasets):
    print('Prepare numpy data:')
    for dataset_name, complex_dataset in datasets.items():
        print('  Prepare numpy data for dataset "%s"' % dataset_name)
        for target in ['positive', 'negative']:
            for selection in complex_dataset[target]:
                selection_size = len(selection['indexes'])
                if selection_size == 0:
                    selection['sentence_vectors'] = np.array([])
                    selection['labels'] = np.array([])
                    continue
                vectors = _create_sentence_vectors(input_description, selection)
                input_description['sentence']['vector_size'] = vectors.shape[1]
                selection['sentence_vectors'] = vectors
                selection['labels'] = np.full(selection_size, target == 'positive')
