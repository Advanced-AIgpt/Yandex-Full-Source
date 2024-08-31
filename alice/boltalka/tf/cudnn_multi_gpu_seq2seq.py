import numpy as np
import tensorflow as tf

import data_utils
from cudnn_seq2seq import CudnnSeq2SeqModel

class CudnnMultiGpuSeq2SeqModel(object):
    def __init__(self,
                 dct,
                 embedding_size,
                 lstm_size,
                 num_layers,
                 max_gradient_norm,
                 max_input_sequence_length,
                 max_output_sequence_length,
                 gpus,
                 softmax_num_samples=0,
                 optimizer=None):

        self.models = []
        with tf.variable_scope('cudnn_multi_gpu_seq2seq') as vs, tf.device('/cpu:0'):
            for gpu in gpus:
                with tf.name_scope('gpu{}'.format(gpu)) as ns:
                    with tf.device('/gpu:{}'.format(gpu)):
                        model = CudnnSeq2SeqModel(dct=dct,
                                                  embedding_size=embedding_size,
                                                  lstm_size=lstm_size,
                                                  num_layers=num_layers,
                                                  max_gradient_norm=max_gradient_norm,
                                                  max_input_sequence_length=max_input_sequence_length,
                                                  max_output_sequence_length=max_output_sequence_length,
                                                  softmax_num_samples=softmax_num_samples,
                                                  optimizer=tf.train.GradientDescentOptimizer(learning_rate=1.0),
                                                  mean_batch_loss=False)
                    self.models.append(model)

                vs.reuse_variables()

            self.global_step = self.models[0].global_step
            self.batch_size = tf.reduce_sum(tf.pack([model.batch_size for model in self.models]))
            self.batch_size = tf.cast(self.batch_size, tf.float32)

            with tf.name_scope('multi_gpu_loss'):
                model_losses = tf.pack([model.loss for model in self.models])
                self.loss = tf.reduce_sum(model_losses) / self.batch_size

                self.sampled_loss = None
                if softmax_num_samples > 0:
                    model_sampled_losses = tf.pack([model.sampled_loss for model in self.models])
                    self.sampled_loss = tf.reduce_sum(model_sampled_losses) / self.batch_size

            with tf.name_scope('multi_gpu_gradients'):
                self.optimizer = optimizer

                self.variables = self.models[0].variables
                self.gradients = []
                for gradients in zip(*[model.gradients for model in self.models]):
                    self.gradients.append(tf.reduce_sum(tf.pack(gradients), 0) / self.batch_size)

                self.clipped_gradients = [None] * len(self.gradients)
                for i, grad in enumerate(self.gradients):
                    if grad is not None:
                        self.clipped_gradients[i] = tf.clip_by_norm(grad, max_gradient_norm)

                clipped_gradients_and_variables = zip(self.clipped_gradients, self.variables)
                self.train_op = self.optimizer.apply_gradients(clipped_gradients_and_variables, global_step=self.global_step)

            self.saver = tf.train.Saver(tf.all_variables(), max_to_keep=2)

    def _get_training_loss(self):
        return self.loss if self.sampled_loss is None else self.sampled_loss

    def _get_input_feed(self, input_data):
        input_feed = {}
        for i, model in enumerate(self.models):
            batch_start = i * len(input_data) // len(self.models)
            batch_end = (i + 1) * len(input_data) // len(self.models)
            part_input_feed = model._get_input_feed(input_data[batch_start:batch_end])
            input_feed.update(part_input_feed)
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
        # TODO(alipov): make parallel implementation
        return self.models[0].predict_proba(sess, input_data)

if __name__ == '__main__':
    with open('empty.dict', 'w') as out:
        pass
    from data_utils import *
    dct, _ = load_dictionary('empty.dict')

    model = CudnnMultiGpuSeq2SeqModel(dct=dct,
                                      embedding_size=36,
                                      lstm_size=123,
                                      num_layers=3,
                                      max_gradient_norm=0.9,
                                      max_input_sequence_length=5,
                                      max_output_sequence_length=6,
                                      softmax_num_samples=3,
                                      gpus=[0, 1],
                                      optimizer=tf.train.AdamOptimizer(0.05, epsilon=1e-2))

    print 'Created model!'
    config = tf.ConfigProto()
    config.log_device_placement = False
    config.gpu_options.allow_growth = True
    config.allow_soft_placement = True

    input_data = [
        [[3, 3, 3, 2], [3, 2]],
        [[3, 3, 3, 3, 2], [3, 3, 3, 3, 2]],
        [[3, 3, 2], [3, 3, 2]]
    ]

    with tf.Session(config=config) as sess:
        for v in tf.all_variables():
            print v.op.name

        sess.run(tf.initialize_all_variables())
        for train_it in xrange(1000):
            loss, _ = model(sess, input_data)
            print train_it, loss
        print model(sess, input_data, is_training=False)

