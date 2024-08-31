# coding: utf-8

import argparse
import os
import numpy as np
import logging
import json

from evaluate_predictions import evaluate
from model import AnaphoraModelApplier, AnaphoraModelTrainer
from utils import Parameters, Example, BatchGenerator, SPECIAL_SYMBOLS, BOS_INDEX, EOS_INDEX, SEP_INDEX, MAX_ENTITY_LEN

logger = logging.getLogger(__name__)


def _generate_segment_ids(sequence):
    segment_ids = []
    segment_id = 1
    for token in reversed(sequence):
        segment_ids.append(segment_id)
        if token == SPECIAL_SYMBOLS[SEP_INDEX]:
            segment_id += 1
    return list(reversed(segment_ids))


def _generate_entity_possible_positions_in_phrase(tokens, shift):
    positions = []
    for i in range(0, len(tokens)):
        for j in range(1, MAX_ENTITY_LEN + 1):
            if i + j <= len(tokens):
                positions.append((shift + i, shift + i + j))
    return positions


def _generate_entity_possible_positions(discussion):
    shift = 0
    tokens = []
    positions = []
    for token in discussion:
        if token != '[SEP]':
            tokens.append(token)
        else:
            positions.extend(_generate_entity_possible_positions_in_phrase(tokens, shift))
            shift += len(tokens) + 1
            tokens = []
    # The last phrase is intentionally skipped. Change the behaviour when pronoun extraction is required
    return positions


def _read_examples(path, word_to_index):
    examples = []

    with open(path) as f:
        for line in f:
            tokens, positions, entity_labels, entity_texts, additional_labels = line.strip().split('\t')[:5]

            tokens = [SPECIAL_SYMBOLS[BOS_INDEX]] + tokens.split() + [SPECIAL_SYMBOLS[EOS_INDEX]]
            segment_ids = _generate_segment_ids(tokens)

            token_ids = [word_to_index.get(token, 0) for token in tokens]
            positions = [(int(pos.split(',')[0]), int(pos.split(',')[1])) for pos in positions.split()]
            positions = [(start + 1, end + 1) for (start, end) in positions]

            entity_positions, pronoun_position = positions[:-1], positions[-1]
            entity_labels = [int(label) for label in entity_labels.split()]
            correct_entity_positions = [
                position for position, label in zip(entity_positions, entity_labels) if label == 1
            ]
            correct_entity_texts = [
                text for text, label in zip(entity_texts.split(', '), entity_labels) if label == 1
            ]

            possible_entity_positions = _generate_entity_possible_positions(tokens)

            for position, text in zip(correct_entity_positions, correct_entity_texts):
                if position not in possible_entity_positions:
                    logger.warning('Skipped correct entity %s', ' '.join(tokens[i] for i in range(*position)))
                else:
                    result_text = ' '.join(tokens[i] for i in range(*position))
                    assert text == result_text, 'Incorrect entity text. Expected {}, found {}'.format(text, result_text)

            entity_labels = [int(position in correct_entity_positions) for position in possible_entity_positions]
            nothing, in_same_request, only_in_context = [int(label) for label in additional_labels.split()]

            if only_in_context:
                logger.warning('Skipped sample with only_in_context label "%s"', ' '.join(tokens))
                continue

            positions = possible_entity_positions + [pronoun_position]

            examples.append(
                Example(tokens=tokens, token_ids=token_ids, segment_ids=segment_ids,
                        positions=positions, entity_labels=entity_labels,
                        phrase_level_labels=[nothing, in_same_request])
            )

    return examples


def _get_embeddings(embeddings_dir):
    embeddings_matrix = np.load(os.path.join(embeddings_dir, 'embeddings.npy'))
    embeddings_matrix = np.concatenate((np.zeros((len(SPECIAL_SYMBOLS), 300)), embeddings_matrix), 0)
    with open(os.path.join(embeddings_dir, 'embeddings.dict')) as f:
        index_to_word = SPECIAL_SYMBOLS + [line.rstrip() for line in f]
        word_to_index = {word: index for index, word in enumerate(index_to_word)}

    return embeddings_matrix, index_to_word, word_to_index


def _train(params, data_path, output_path):
    embeddings_matrix, _, word_to_index = _get_embeddings(data_path)

    train_data = []
    for train_data_name in params.train_data:
        train_data.extend(_read_examples(os.path.join(data_path, train_data_name), word_to_index))

    one_count, total_count, has_one = 0., 0, 0.
    for example in train_data:
        one_count += sum(example.entity_labels)
        has_one += int(sum(example.entity_labels) > 0)
        total_count += len(example.entity_labels)

    logger.info('Ones ratio = %s', one_count / total_count)
    logger.info('Has entities = %s', has_one / len(train_data))

    logger.info('Sample Example = %s', train_data[0])
    logger.info('Train data size = %s', len(train_data))

    train_generator = BatchGenerator(train_data)
    logger.info('Sample Batch = %s', next(iter(train_generator)))

    test_data = _read_examples(os.path.join(data_path, params.test_data), word_to_index)
    test_generator = BatchGenerator(test_data)

    trainer = AnaphoraModelTrainer(params=params, embeddings_matrix=embeddings_matrix)

    try:
        trainer.fit(
            train_batch_generator=train_generator,
            val_batch_generator=test_generator,
            epochs_count=params.epoch_count
        )
    except KeyboardInterrupt:
        logger.info('Early stopping triggered')

    trainer.save(output_path)


def _evaluate(train_config, data_path, output_path):
    embeddings_matrix, _, word_to_index = _get_embeddings(data_path)
    model = AnaphoraModelApplier(output_path, embeddings_matrix, word_to_index)

    test_data_path = os.path.join(data_path, train_config.test_data)
    predictions_output_path = os.path.join(output_path, 'preds.tsv')

    with open(test_data_path) as f, open(predictions_output_path, 'w') as f_out:
        for line in f:
            tokens, positions, entity_labels, entity_texts, additional_labels = line.strip().split('\t')[:5]
            tokens = tokens.split()
            session = [[]]
            for token in tokens:
                if token != SPECIAL_SYMBOLS[SEP_INDEX]:
                    session[-1].append(token)
                else:
                    session.append([])

            if additional_labels.split()[1] == '1':
                correct_entities = ''
            else:
                entity_labels = map(int, entity_labels.split())
                entity_texts = entity_texts.split(', ')[:-1]
                correct_entities = ', '.join(
                    entity for entity, label in zip(entity_texts, entity_labels)
                    if label == 1
                )

            possible_entity_positions = _generate_entity_possible_positions(tokens)
            positions = [(int(pair.split(',')[0]), int(pair.split(',')[1])) for pair in positions.split()]
            correct_positions = [position for position, label in zip(positions[:-1], entity_labels) if label == 1]
            positions = possible_entity_positions + [positions[-1]]
            positions = [(start + 1, end + 1) for (start, end) in positions]

            entity_positions, pronoun_position = positions[:-1], positions[-1]

            predicted_entity, predicted_entity_debug, max_prob, correct_entity_prob = model.predict(
                session, entity_positions, pronoun_position, correct_positions
            )
            f_out.write('{}\t{}\t{}\t{}\t{}\t{}\n'.format(
                ' '.join(tokens), correct_entities, predicted_entity or '',
                predicted_entity_debug, max_prob, correct_entity_prob
            ))

    evaluate(predictions_output_path, None, None)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--skip-train', action='store_true', default=False)
    parser.add_argument('--data-path', default='data')
    parser.add_argument('--output-path', default='models')

    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(script_dir, 'train_config.json')) as f:
        train_config = json.load(f)

    params = Parameters(**train_config)

    logger.info('Train config:\n%s', params)

    if not args.skip_train:
        _train(params, args.data_path, args.output_path)
    _evaluate(params, args.data_path, args.output_path)


if __name__ == '__main__':
    main()
