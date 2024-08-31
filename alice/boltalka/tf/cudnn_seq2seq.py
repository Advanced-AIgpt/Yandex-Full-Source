import numpy as np
import tensorflow as tf

import data_utils
from cudnn_wrapper import CanonicalCudnnLstm as CudnnLstm

class CudnnSeq2SeqModel(object):
    def __init__(self,
                 dct,
                 embedding_size,
                 lstm_size,
                 num_layers,
                 max_gradient_norm,
                 max_input_sequence_length,
                 max_output_sequence_length,
                 optimizer,
                 softmax_num_samples=0,
                 mean_batch_loss=True):

        self.dct = dct
        dict_size = len(dct)

        self.max_input_sequence_length = max_input_sequence_length
        self.max_output_sequence_length = max_output_sequence_length

        with tf.variable_scope('cudnn_seq2seq', dtype=tf.float32):

            self.global_step = tf.get_variable('global_step',
                                               shape=[],
                                               initializer=tf.constant_initializer(0),
                                               trainable=False,
                                               dtype=tf.int32)

            self.embeddings = tf.get_variable('embeddings',
                                              shape=[dict_size, embedding_size],
                                              initializer=tf.uniform_unit_scaling_initializer())

            with tf.variable_scope('encoder'):
                # sequence_length x batch_size
                self.encoder_inputs = tf.placeholder(tf.int32, shape=[max_input_sequence_length, None], name='inputs')
                # sequence_length x batch_size x embedding_size
                self.encoder_input_data = tf.gather(self.embeddings, self.encoder_inputs)
                self.sequences_lengths = tf.placeholder(tf.int32, shape=[None], name='sequences_lengths')

                self.encoder = CudnnLstm(num_layers=num_layers,
                                         num_units=lstm_size,
                                         input_size=embedding_size,
                                         weights_initializer=tf.uniform_unit_scaling_initializer(),
                                         biases_initializer=tf.constant_initializer(0.0),
                                         forget_bias=0.0)

                self.encoder_outputs = self.encoder(input_data=self.encoder_input_data,
                                                    sequences_lengths=self.sequences_lengths)

            with tf.variable_scope('decoder'):
                # sequence_length x batch_size
                self.decoder_inputs = tf.placeholder(tf.int32, shape=[max_output_sequence_length + 1, None], name='inputs')
                # sequence_length x batch_size x embedding_size
                self.decoder_input_data = tf.concat(2,
                    [
                        tf.gather(self.embeddings, self.decoder_inputs[:-1, :]),
                        tf.tile(tf.expand_dims(self.encoder_outputs, 0), [max_output_sequence_length, 1, 1])
                    ])

                self.target_weights = tf.placeholder(tf.float32, shape=[max_output_sequence_length, None], name='target_weights')
                self.targets = self.decoder_inputs[1:, :]

                self.decoder = CudnnLstm(num_layers=num_layers,
                                         num_units=lstm_size,
                                         input_size=embedding_size + lstm_size,
                                         weights_initializer=tf.uniform_unit_scaling_initializer(),
                                         biases_initializer=tf.constant_initializer(0.0),
                                         forget_bias=0.0)

                self.decoder_outputs = self.decoder(input_data=self.decoder_input_data)
                batch_size = tf.shape(self.decoder_outputs)[1]

                intermediate_projection_size = lstm_size // 4
                with tf.variable_scope('intermediate_projection'):
                    self.intermediate_projection_weights = tf.get_variable('weights',
                                                                           shape=[lstm_size, intermediate_projection_size],
                                                                           initializer=tf.uniform_unit_scaling_initializer())
                    self.intermediate_projection_biases = tf.get_variable('biases',
                                                                          shape=[intermediate_projection_size],
                                                                          initializer=tf.constant_initializer(0.0))

                    batched_decoder_outputs = tf.transpose(self.decoder_outputs, perm=[1, 0, 2])
                    batched_weights = tf.tile(tf.expand_dims(self.intermediate_projection_weights, 0), [batch_size, 1, 1])
                    batched_biases = tf.tile(tf.expand_dims(tf.expand_dims(self.intermediate_projection_biases, 0), 0), [batch_size, max_output_sequence_length, 1])

                    # batch_size x sequence_length x intermediate_projection_size
                    self.intermediate_projection = tf.tanh(
                        tf.batch_matmul(batched_decoder_outputs, batched_weights) + batched_biases)

                with tf.variable_scope('output_projection'):
                    self.output_projection_weights_t = tf.get_variable('weights',
                                                                       shape=[dict_size, intermediate_projection_size],
                                                                       initializer=tf.uniform_unit_scaling_initializer())
                    self.output_projection_biases = tf.get_variable('biases',
                                                                    shape=[dict_size],
                                                                    initializer=tf.constant_initializer(0.0))

                    batched_weights = tf.tile(tf.expand_dims(tf.transpose(self.output_projection_weights_t), 0), [batch_size, 1, 1])
                    batched_biases = tf.tile(tf.expand_dims(tf.expand_dims(self.output_projection_biases, 0), 0), [batch_size, max_output_sequence_length, 1])

                    # batch_size x sequence_length x dict_size
                    self.output_projection = tf.batch_matmul(self.intermediate_projection, batched_weights) + batched_biases

            with tf.name_scope('loss') as ns:
                self.loss = tf.nn.sparse_softmax_cross_entropy_with_logits(self.output_projection, tf.transpose(self.targets))
                self.loss = tf.reduce_sum(self.loss * tf.transpose(self.target_weights), 1)
                if mean_batch_loss:
                    self.loss = tf.reduce_mean(self.loss)
                else:
                    self.loss = tf.reduce_sum(self.loss)

                self.sampled_loss = None
                if softmax_num_samples > 0:
                    self.sampled_loss = tf.nn.sampled_softmax_loss(
                        self.output_projection_weights_t,
                        self.output_projection_biases,
                        tf.reshape(self.intermediate_projection, [-1, intermediate_projection_size]),
                        tf.reshape(tf.transpose(self.targets), [-1, 1]),
                        softmax_num_samples,
                        dict_size)
                    self.sampled_loss = tf.reshape(self.sampled_loss, [batch_size, -1])
                    self.sampled_loss = tf.reduce_sum(self.sampled_loss * tf.transpose(self.target_weights), 1)
                    if mean_batch_loss:
                        self.sampled_loss = tf.reduce_mean(self.sampled_loss)
                    else:
                        self.sampled_loss = tf.reduce_sum(self.sampled_loss)

            with tf.variable_scope('gradients'):
                self.optimizer = optimizer

                gradients_and_variables = self.optimizer.compute_gradients(self._get_training_loss())
                self.gradients = [grad for grad, var in gradients_and_variables]
                self.variables = [var for grad, var in gradients_and_variables]

                self.clipped_gradients = [None] * len(self.gradients)
                for i, grad in enumerate(self.gradients):
                    if grad is not None:
                        self.clipped_gradients[i] = tf.clip_by_norm(grad, max_gradient_norm)

                clipped_gradients_and_variables = zip(self.clipped_gradients, self.variables)
                self.train_op = self.optimizer.apply_gradients(clipped_gradients_and_variables, global_step=self.global_step)

            self.batch_size = batch_size
            self.saver = tf.train.Saver(tf.all_variables(), max_to_keep=2)

    def _get_training_loss(self):
        return self.loss if self.sampled_loss is None else self.sampled_loss

    def _pad(self, data, length, pad):
        return data +  [pad] * (length - len(data))

    def _reshape_as_time_major(self, data, dtype):
        return np.array(data, dtype=dtype).T

    def _get_batch(self, input_data):
        pad = self.dct[data_utils._PAD]
        go = self.dct[data_utils._GO]

        encoder_inputs = []
        sequences_lengths = []
        decoder_inputs = []
        target_weights = []

        for context, reply in input_data:
            context = context[-self.max_input_sequence_length:]
            encoder_inputs.append(self._pad(context, self.max_input_sequence_length, pad))
            sequences_lengths.append(len(context))

            reply = reply[:self.max_output_sequence_length]
            decoder_inputs.append([go] + self._pad(reply, self.max_output_sequence_length, pad))
            target_weights.append(self._pad([1] * len(reply), self.max_output_sequence_length, 0))

        encoder_inputs = self._reshape_as_time_major(encoder_inputs, np.int32)
        sequences_lengths = np.array(sequences_lengths, dtype=np.int32)
        decoder_inputs = self._reshape_as_time_major(decoder_inputs, np.int32)
        target_weights = self._reshape_as_time_major(target_weights, np.float32)

        return encoder_inputs, sequences_lengths, decoder_inputs, target_weights

    def _get_input_feed(self, input_data):
        encoder_inputs, sequences_lengths, decoder_inputs, target_weights = self._get_batch(input_data)

        input_feed = {}
        input_feed[self.encoder_inputs.name] = encoder_inputs
        input_feed[self.sequences_lengths.name] = sequences_lengths
        input_feed[self.decoder_inputs.name] = decoder_inputs
        input_feed[self.target_weights.name] = target_weights
        return input_feed

    def __call__(self, sess, input_data, is_training=True):
        input_feed = self._get_input_feed(input_data)

        output_feed = []
        if is_training:
            output_feed.append(self._get_training_loss())
            output_feed.append(self.train_op)
        else:
            output_feed.append(self.loss)

        return sess.run(output_feed, input_feed)

    def predict_proba(self, sess, input_data):
        input_feed = self._get_input_feed(input_data)
        # batch_size x sequence_length x dict_size
        batch_size = tf.shape(self.output_projection)[0]
        sequence_length = tf.shape(self.output_projection)[1]

        targets = tf.expand_dims(tf.transpose(self.targets), -1)
        targets = tf.concat(2, [tf.reshape(tf.range(batch_size * sequence_length), [batch_size, -1, 1]), targets])
        targets = tf.reshape(targets, [-1, 2])

        probas = tf.nn.softmax(self.output_projection)
        probas = tf.reshape(probas, [-1, len(self.dct)])
        probas = tf.gather_nd(probas, targets)
        probas = tf.reshape(probas, [batch_size, -1])

        probas = sess.run(probas, input_feed)

        result = []
        for i, (context, reply) in enumerate(input_data):
            result.append(probas[i, :len(reply)])
        return result

if __name__ == '__main__':
    with open('empty.dict', 'w') as out:
        pass
    from data_utils import *
    dct = load_dictionary('empty.dict')

    model = CudnnSeq2SeqModel(dct=dct,
                              embedding_size=36,
                              lstm_size=123,
                              num_layers=3,
                              max_gradient_norm=0.9,
                              max_input_sequence_length=5,
                              max_output_sequence_length=6,
                              softmax_num_samples=3)

    print 'Created model!'
    config = tf.ConfigProto()
    config.log_device_placement = False
    config.gpu_options.allow_growth = True

    input_data = [
        [[3, 3, 3, 2], [3, 2]],
        [[3, 3, 3, 3, 2], [3, 3, 3, 3, 2]]
    ]

    with tf.Session(config=config) as sess:
        for v in tf.all_variables():
            print v.op.name
        sess.run(tf.initialize_all_variables())
        for train_it in xrange(10):
            loss, _ = model(sess, input_data)
            print train_it, loss
        print model(sess, input_data, is_training=False)
