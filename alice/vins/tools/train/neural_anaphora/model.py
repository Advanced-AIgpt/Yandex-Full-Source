# coding: utf-8

import os
import numpy as np
import logging
import json
import tempfile
import shutil
import tensorflow as tf

from tensorflow.contrib.rnn import LSTMBlockFusedCell, TimeReversedFusedRNN, transpose_batch_time

from utils import F1Counter, Batch, SPECIAL_SYMBOLS, PAD_INDEX, SEP_INDEX, BOS_INDEX, EOS_INDEX

logger = logging.getLogger(__name__)


class AnaphoraModelTrainer(object):
    def __init__(self, params, embeddings_matrix=None, is_train_model=True, logs_path='logs'):
        assert not is_train_model or params.word_emb_dim is not None
        assert is_train_model or embeddings_matrix is not None

        self._params = params
        self._embeddings_matrix = embeddings_matrix
        self._is_train_model = is_train_model
        self._logs_path = logs_path

        self._build_model()

    def _build_model(self):
        with tf.Graph().as_default() as graph:
            self._is_training = tf.placeholder_with_default(False, shape=(), name='training')
            if self._is_train_model:
                self._sequence_lengths = tf.placeholder(tf.int32, shape=[None], name='sequence_lengths')

            inputs = [self._add_word_inputs()]
            if self._params.use_segment_embeddings:
                inputs.append(self._add_segment_inputs())
            inputs = tf.concat(inputs, -1)

            with tf.variable_scope('encoder'):
                inputs = transpose_batch_time(inputs)
                for layer_ind in xrange(self._params.encoder_num_layers):
                    outputs, last_states = self._add_encoder(inputs, layer_ind)
                    inputs = tf.concat(outputs, -1)

                fw_embs, bw_embs = transpose_batch_time(outputs[0]), transpose_batch_time(outputs[1])
                entity_embs, phrase_embs = self._extract_embeddings(fw_embs, bw_embs, last_states)

                if self._params.use_distance_embeddings:
                    entity_embs = tf.concat([entity_embs, self._get_distance_embeddings()], -1)

            with tf.variable_scope('classifier'):
                self._add_outputs(entity_embs, phrase_embs)

            if self._is_train_model:
                self._add_loss()
                self._add_optimizer()

            self._summary = tf.summary.merge_all()

            self._train_writer = tf.summary.FileWriter(os.path.join(self._logs_path, 'train'), graph)
            self._val_writer = tf.summary.FileWriter(os.path.join(self._logs_path, 'val'))

            self._init_session(graph)
            self._init_saver(graph)

    def _add_word_inputs(self):
        if not self._is_train_model:
            with tf.variable_scope('inference'):
                self._words = tf.placeholder(
                    tf.float32, shape=[None, None, self._params.word_emb_dim], name='word_embs'
                )
                return self._words

        with tf.variable_scope('train'):
            self._words = tf.placeholder(tf.int32, shape=[None, None], name='words')

            frozen_embeddings_var = tf.Variable(
                initial_value=self._embeddings_matrix, dtype=tf.float32, trainable=False, name='embedding_matrix'
            )
            word_embs = tf.nn.embedding_lookup(frozen_embeddings_var, self._words)

            self._trainable_embeddings_var = tf.get_variable(
                name='trainable_embeddings', shape=[len(SPECIAL_SYMBOLS) - 1, self._embeddings_matrix.shape[-1]]
            )

            trainable_embeddings = tf.concat(
                [tf.zeros((1, self._embeddings_matrix.shape[-1])), self._trainable_embeddings_var], 0
            )
            special_symbol_positions = tf.where(
                self._words < len(SPECIAL_SYMBOLS), self._words, tf.zeros_like(self._words)
            )
            word_embs += tf.nn.embedding_lookup(trainable_embeddings, special_symbol_positions)

            self._word_embs = word_embs

            return word_embs

    def _add_segment_inputs(self):
        self._segment_ids = tf.placeholder(tf.int32, shape=[None, None], name='segment_ids')
        segment_embeddings_var = tf.get_variable(
            name='segment_embeddings', shape=[6, self._params.feature_embedding_size]
        )

        return tf.nn.embedding_lookup(segment_embeddings_var, self._segment_ids)

    def _add_speaker_inputs(self):
        segment_embeddings_var = tf.get_variable(
            name='speaker_embeddings', shape=[2, self._params.feature_embedding_size]
        )

        speaker_ids = tf.mod(self._segment_ids, 2)
        return tf.nn.embedding_lookup(segment_embeddings_var, speaker_ids)

    def _add_encoder(self, inputs, layer_ind):
        with tf.variable_scope('encoder_{}'.format(layer_ind)):
            if self._is_train_model:
                inputs = tf.layers.dropout(inputs, rate=self._params.dropout_rate, training=self._is_training,
                                           noise_shape=tf.concat([[1], tf.shape(inputs)[1:]], axis=0))

            with tf.variable_scope('forward') as forward_scope:
                cell = LSTMBlockFusedCell(num_units=self._params.encoder_hidden_dim / 2)
                output_fw, (_, last_output_fw) = cell(
                    inputs, sequence_length=self._sequence_lengths if self._is_train_model else None,
                    dtype=tf.float32, scope=forward_scope
                )

            with tf.variable_scope('backward') as backward_scope:
                cell = TimeReversedFusedRNN(LSTMBlockFusedCell(num_units=self._params.encoder_hidden_dim / 2))
                output_bw, (_, last_output_bw) = cell(
                    inputs, sequence_length=self._sequence_lengths if self._is_train_model else None,
                    dtype=tf.float32, scope=backward_scope
                )

            return [output_fw, output_bw], [last_output_fw, last_output_bw]

    def _extract_embeddings(self, fw_embs, bw_embs, last_states):
        self._pronoun_rows = tf.placeholder_with_default([0], shape=[None], name='pronoun_rows')
        self._entity_rows = tf.placeholder(tf.int32, shape=[None], name='entity_rows')

        self._pronoun_start_positions = tf.placeholder(tf.int32, shape=[None], name='pronoun_start_positions')
        self._pronoun_end_positions = tf.placeholder(tf.int32, shape=[None], name='pronoun_end_positions')
        self._entity_start_positions = tf.placeholder(tf.int32, shape=[None], name='entity_start_positions')
        self._entity_end_positions = tf.placeholder(tf.int32, shape=[None], name='entity_end_positions')

        pronoun_embs = [
            tf.gather_nd(fw_embs, tf.stack([self._pronoun_rows, self._pronoun_end_positions - 1], 1)),
            tf.gather_nd(bw_embs, tf.stack([self._pronoun_rows, self._pronoun_start_positions], 1)),
            tf.gather_nd(fw_embs, tf.stack([self._pronoun_rows, self._pronoun_start_positions - 1], 1)),
            tf.gather_nd(bw_embs, tf.stack([self._pronoun_rows, self._pronoun_end_positions], 1)),
        ]

        pronoun_embs = tf.concat(pronoun_embs, -1)
        repeated_pronoun_embs = tf.gather(pronoun_embs, self._entity_rows)

        entity_embs = [
            tf.gather_nd(fw_embs, tf.stack([self._entity_rows, self._entity_end_positions - 1], 1)),
            tf.gather_nd(bw_embs, tf.stack([self._entity_rows, self._entity_start_positions], 1)),
            tf.gather_nd(fw_embs, tf.stack([self._entity_rows, self._entity_start_positions - 1], 1)),
            tf.gather_nd(bw_embs, tf.stack([self._entity_rows, self._entity_end_positions], 1)),
        ]

        if self._params.use_speaker_embeddings:
            speaker_embs = self._add_speaker_inputs()
            speaker_embs = tf.gather_nd(speaker_embs, tf.stack([self._entity_rows, self._entity_end_positions], 1))
            entity_embs.append(speaker_embs)

        entity_embs = tf.concat(entity_embs, -1)

        entity_embs = tf.concat([repeated_pronoun_embs, entity_embs], -1)
        phrase_embs = tf.concat(last_states + [pronoun_embs], -1)

        return entity_embs, phrase_embs

    def _get_distance_embeddings(self):
        pronoun_positions = tf.gather(self._pronoun_start_positions, self._entity_rows)
        entity_positions = self._entity_end_positions - 1
        distances = pronoun_positions - entity_positions

        buckets = [(0, 4), (4, 8), (8, 16), (16, 32), (32, 64), (64, 1024)]

        def _get_bucket_index(i):
            for bucket_index, (start, end) in enumerate(buckets):
                if start <= i < end:
                    return bucket_index

        buckets_var = tf.constant([_get_bucket_index(i) for i in xrange(1024)], dtype=tf.int32)
        bucketed_distances = tf.nn.embedding_lookup(buckets_var, distances)
        bucket_embeddings_var = tf.get_variable(
            name='distance_embeddings', shape=[len(buckets), self._params.feature_embedding_size]
        )
        return tf.nn.embedding_lookup(bucket_embeddings_var, bucketed_distances)

    def _add_outputs(self, entity_embs, phrase_embs):
        entity_embs = tf.layers.dropout(entity_embs, rate=self._params.dropout_rate)
        phrase_embs = tf.layers.dropout(phrase_embs, rate=self._params.dropout_rate)

        self._entity_logits = tf.squeeze(self._add_output_classifier(entity_embs, 1), -1)
        self._entity_probs = tf.nn.sigmoid(self._entity_logits)

        self._phrase_logits = self._add_output_classifier(phrase_embs, 2)
        self._phrase_probs = tf.nn.sigmoid(self._phrase_logits)

    def _add_output_classifier(self, inputs, output_dim):
        for _ in xrange(self._params.classifier_num_layers):
            inputs = tf.layers.dense(inputs, self._params.classifier_hidden_dim, activation=tf.nn.relu)
        return tf.layers.dense(inputs, output_dim)

    def _add_softmax_output(self):
        if not self._is_train_model:
            self._entity_logprobs = self._entity_logits - tf.reduce_logsumexp(self._entity_logits)
        else:
            entity_logits = self._entity_logits - tf.reduce_max(self._entity_logits)
            entity_logits_exp = tf.exp(entity_logits)

            denominator = tf.scatter_nd(
                indices=tf.expand_dims(self._entity_rows, 1),
                updates=entity_logits_exp,
                shape=tf.shape(self._pronoun_rows)
            )

            denominator = tf.gather(denominator, self._entity_rows)
            denominator = tf.log(denominator + 1e-9)

            self._entity_logprobs = entity_logits - denominator

    def _add_loss(self):
        self._entity_labels = tf.placeholder(tf.int32, shape=[None], name='entity_labels')
        self._phrase_level_labels = tf.placeholder(tf.int32, shape=[None, 2], name='phrase_labels')

        with tf.variable_scope('loss'):
            self._loss = 0.
            if self._params.query_softmax_loss:
                self._loss += self._add_query_softmax_loss()

            if self._params.bce_loss:
                weights = self._params.positive_class_weight * self._entity_labels + 1
                self._loss += tf.losses.sigmoid_cross_entropy(
                    multi_class_labels=self._entity_labels,
                    logits=self._entity_logits,
                    weights=tf.cast(weights, tf.float32)
                )

            self._loss += tf.losses.sigmoid_cross_entropy(
                multi_class_labels=self._phrase_level_labels,
                logits=self._phrase_logits
            )

            tf.summary.scalar('loss', self._loss)

    def _add_query_softmax_loss(self):
        entity_labels = tf.cast(self._entity_labels, tf.float32)
        entity_logprobs = entity_labels * self._entity_logprobs

        return -tf.reduce_sum(entity_logprobs) / (tf.reduce_sum(entity_labels) + 1e-9)

    def _add_optimizer(self):
        with tf.variable_scope('optimizer'):
            optimizer = tf.train.AdamOptimizer()

            grads, variables = zip(*optimizer.compute_gradients(self._loss))
            grads, _ = tf.clip_by_global_norm(grads, 5.)
            self._train_step = optimizer.apply_gradients(zip(grads, variables))

    def _init_session(self, graph):
        config = tf.ConfigProto()
        config.gpu_options.allow_growth = True
        self._sess = tf.Session(config=config, graph=graph)
        self._sess.run(tf.global_variables_initializer())

    def _init_saver(self, graph):
        saveable_vars = (
            graph.get_collection(tf.GraphKeys.GLOBAL_VARIABLES) + graph.get_collection(tf.GraphKeys.SAVEABLE_OBJECTS)
        )
        saveable_vars = [variable for variable in saveable_vars if not 'train' in variable.name]
        self._saver = tf.train.Saver(var_list=saveable_vars)

    @property
    def inputs(self):
        return [
            self._words,
            self._segment_ids,
            self._pronoun_start_positions,
            self._pronoun_end_positions,
            self._entity_rows,
            self._entity_start_positions,
            self._entity_end_positions,
        ]

    @property
    def outputs(self):
        outputs = [
            self._entity_probs,
            self._phrase_probs,
        ]
        if self._params.query_softmax_loss:
            outputs.append(self._entity_logprobs)
        return outputs

    def fit(self, train_batch_generator, val_batch_generator=None, epochs_count=10):
        for epoch in xrange(epochs_count):
            self._run_epoch(epoch, epochs_count, batch_generator=train_batch_generator, is_train_epoch=True)
            if val_batch_generator is not None:
                self._run_epoch(
                    epoch, epochs_count, batch_generator=val_batch_generator, is_train_epoch=False
                )

    def _run_epoch(self, epoch, epochs_count, batch_generator, is_train_epoch):
        batches_count = len(batch_generator)
        train_step = [self._train_step] if is_train_epoch else []

        total_loss = 0.
        fscores = [F1Counter() for _ in xrange(3)]
        writer = self._train_writer if is_train_epoch else self._val_writer
        name = 'Train' if is_train_epoch else '  Val'
        for i, batch in enumerate(batch_generator):
            assert isinstance(batch, Batch)

            feed = {
                self._is_training: is_train_epoch,
                self._sequence_lengths: batch.lengths,
                self._words: batch.token_ids,
                self._pronoun_rows: batch.pronoun_rows,
                self._entity_rows: batch.entity_rows,
                self._pronoun_start_positions: batch.pronoun_start_positions,
                self._pronoun_end_positions: batch.pronoun_end_positions,
                self._entity_start_positions: batch.entity_start_positions,
                self._entity_end_positions: batch.entity_end_positions,
                self._entity_labels: batch.entity_labels,
                self._phrase_level_labels: batch.phrase_level_labels
            }
            if self._params.use_segment_embeddings:
                feed[self._segment_ids] = batch.segment_ids

            summary, loss, entity_logits, phrase_logits = self._sess.run(
                [self._summary, self._loss, self._entity_logits, self._phrase_logits] + train_step, feed_dict=feed
            )[:4]
            total_loss += loss

            writer.add_summary(summary, i + epoch * batches_count)

            fscores[0].update(entity_logits, batch.entity_labels)
            fscores[1].update(phrase_logits[:, 0], batch.phrase_level_labels[:, 0])
            fscores[2].update(phrase_logits[:, 1], batch.phrase_level_labels[:, 1])

            if i + 1 == batches_count:
                break

        logger.info('[{} / {}] {}: Loss = {:.4f}. {}'.format(
            epoch + 1, epochs_count, name, total_loss / batches_count, ', '.join(str(score) for score in fscores)
        ))

    def save(self, save_dir):
        if not os.path.isdir(save_dir):
            os.makedirs(save_dir)

        trainable_embeddings = self._sess.run([self._trainable_embeddings_var])[0]
        trainable_embeddings = np.concatenate(
            (np.zeros((1, trainable_embeddings.shape[-1])), trainable_embeddings), 0
        )
        assert trainable_embeddings.shape == (len(SPECIAL_SYMBOLS), self._embeddings_matrix.shape[-1])

        # save trainable_embeddings for VINS
        np.save(os.path.join(save_dir, 'trainable_embeddings.npy'), trainable_embeddings)

        # save trainable_embeddings for Megamind
        trainable_embeddings_json = {symbol: list(embedding) for symbol, embedding in zip(SPECIAL_SYMBOLS, trainable_embeddings)}
        with open(os.path.join(save_dir, 'special_embeddings.json'), 'w') as f:
            json.dump(trainable_embeddings_json, f, indent=2)

        tmp_dir = None
        try:
            tmp_dir = tempfile.mkdtemp()
            self._saver.save(self._sess, os.path.join(tmp_dir, 'model.chkp'), write_meta_graph=False)

            self._is_train_model = False
            self._build_model()

            self._saver.restore(self._sess, os.path.join(tmp_dir, 'model.chkp'))
        finally:
            if tmp_dir is not None:
                shutil.rmtree(tmp_dir)

        graph_def = self._sess.graph.as_graph_def()
        graph_def = tf.graph_util.convert_variables_to_constants(
            self._sess, graph_def, output_node_names=[node.op.name for node in self.outputs]
        )

        with open(os.path.join(save_dir, 'model.pb'), 'wb') as f:
            f.write(graph_def.SerializeToString())

        # save model description for VINS
        with open(os.path.join(save_dir, 'graph_nodes.txt'), 'w') as f:
            json.dump({
                'inputs': [node.name for node in self.inputs],
                'outputs': [node.name for node in self.outputs]
            }, f, indent=2)

        # save model description for Megamind
        model_description = {
            'inputs': {
                'words': self._words.name,
                'segment_ids': self._segment_ids.name,
                'entity_rows': self._entity_rows.name,
                'pronoun_start_positions': self._pronoun_start_positions.name,
                'pronoun_end_positions': self._pronoun_end_positions.name,
                'entity_start_positions': self._entity_start_positions.name,
                'entity_end_positions': self._entity_end_positions.name
            },
            'outputs': {
                'entity_probs': self._entity_probs.name,
                'phrase_probs': self._phrase_probs.name
            },
            'no_anaphora_probability_threshold': 0.5,
            'anaphora_in_request_probability_threshold': 0.5,
            'entity_probability_threshold': 0.8,
            'max_history_length': 5
        }
        with open(os.path.join(save_dir, 'model_description.json'), 'w') as f:
            json.dump(model_description, f, indent=2)


class AnaphoraModelApplier(object):
    def __init__(self, dir_path, embeddings_matrix, word_to_index):
        self._pretrained_embeddings = embeddings_matrix
        self._word_to_index = word_to_index
        self._tuned_embeddings = np.load(os.path.join(dir_path, 'trainable_embeddings.npy'))

        with open(os.path.join(dir_path, 'graph_nodes.txt')) as f:
            node_names = json.load(f)

        with tf.gfile.GFile(os.path.join(dir_path, 'model.pb'), "rb") as f:
            graph_def = tf.GraphDef()
            graph_def.ParseFromString(f.read())

        with tf.Graph().as_default() as graph:
            self._sess = tf.Session(graph=graph)

            return_elements = tf.import_graph_def(
                graph_def,
                return_elements=node_names['inputs'] + node_names['outputs']
            )

            self._words, self._segment_ids = return_elements[:2]
            self._pronoun_start_positions, self._pronoun_end_positions = return_elements[2: 4]
            self._entity_rows, self._entity_start_positions, self._entity_end_positions = return_elements[4: 7]
            if len(return_elements[7:]) == 2:
                self._entity_probs, self._phrase_level_probs = return_elements[7:]
                self._entity_logprobs = None
            elif len(return_elements[7:]) == 3:
                self._entity_probs, self._phrase_level_probs, self._entity_logprobs = return_elements[7:]

    def predict(self, session, entity_positions, pronoun_position, correct_positions=None):
        assert isinstance(session, list)
        assert isinstance(session[0], list)
        assert isinstance(session[0][0], basestring)
        assert isinstance(entity_positions, list)
        assert isinstance(entity_positions[0], tuple) and len(entity_positions[0]) == 2
        assert isinstance(pronoun_position, tuple) and len(pronoun_position) == 2

        words, word_embeddings, segment_ids = self._get_inputs(session)
        entity_start_positions = np.array([start for start, _ in entity_positions])
        entity_end_positions = np.array([end for _, end in entity_positions])
        correct_positions = [
            entity_positions.index((begin + 1, end + 1))
            for (begin, end) in correct_positions
            if (begin + 1, end + 1) in entity_positions
        ]

        pronoun_start_positions = np.array([pronoun_position[0]])
        pronoun_end_positions = np.array([pronoun_position[1]])

        feed = {
            self._words: np.expand_dims(word_embeddings, 0),
            self._segment_ids: segment_ids,
            self._entity_rows: [0] * len(entity_start_positions),
            self._pronoun_start_positions: pronoun_start_positions,
            self._pronoun_end_positions: pronoun_end_positions,
            self._entity_start_positions: entity_start_positions,
            self._entity_end_positions: entity_end_positions
        }

        if self._entity_logprobs is not None:
            entity_probs, entity_logprobs, phrase_probs = self._sess.run(
                [self._entity_probs, self._entity_logprobs, self._phrase_level_probs], feed_dict=feed
            )
            prob = np.exp(entity_logprobs.max())
            is_good = prob > 0.3
        else:
            entity_probs, phrase_probs = self._sess.run(
                [self._entity_probs, self._phrase_level_probs], feed_dict=feed
            )
            is_good = entity_probs.max() > 0.8

        if correct_positions:
            correct_entity_prob = entity_probs[correct_positions].max()
        else:
            correct_entity_prob = 0.
        max_prob = entity_probs.max()

        predicted_entity_index = entity_probs.argmax()
        predicted_entity_start, predicted_entity_end = entity_positions[predicted_entity_index]
        predicted_entity = ' '.join(
            words[index] for index in xrange(predicted_entity_start, predicted_entity_end)
        )

        if np.all(phrase_probs < 0.5) and is_good:
            if len(words) - len(session[-1]) - 2 <= predicted_entity_start:
                return None, predicted_entity, max_prob, correct_entity_prob

            return predicted_entity, predicted_entity, max_prob, correct_entity_prob

        return None, predicted_entity, max_prob, correct_entity_prob

    def _get_inputs(self, session):
        words, word_embeddings, segment_ids = [], [], []
        word_embeddings.append(self._tuned_embeddings[BOS_INDEX])
        segment_ids.append(len(session))
        words.append(SPECIAL_SYMBOLS[BOS_INDEX])
        for phrase_index, phrase in enumerate(session):
            for word in phrase:
                if word in self._word_to_index:
                    word_embeddings.append(self._pretrained_embeddings[self._word_to_index[word]])
                else:
                    # Unknown word
                    word_embeddings.append(self._tuned_embeddings[PAD_INDEX])
                segment_ids.append(len(session) - phrase_index)

            word_embeddings.append(self._tuned_embeddings[SEP_INDEX])
            segment_ids.append(len(session) - phrase_index)

            words.extend(phrase)
            words.append(SPECIAL_SYMBOLS[SEP_INDEX])

        # Replace [SEP] with [EOS]
        word_embeddings[-1] = self._tuned_embeddings[EOS_INDEX]
        words[-1] = SPECIAL_SYMBOLS[EOS_INDEX]

        word_embeddings = np.stack(word_embeddings, 0)
        segment_ids = np.array(segment_ids, ndmin=2)

        return words, word_embeddings, segment_ids
