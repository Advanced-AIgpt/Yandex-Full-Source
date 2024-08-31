import tensorflow as tf
import numpy as np
import codecs

from recurrent_encoder import Recurrent_encoder
from encoder_general import dense_layer


class Language_model(Recurrent_encoder):
    def __init__(self,
                 word_dct_size,
                 trigram_dct_size,
                 learning_rate,
                 epsilon,
                 grad_threshold,
                 embedding_size=300,
                 hidden_size=300,
                 name='',
                 embedding_scope_name=None):

        super(Language_model, self).__init__(word_dct_size=word_dct_size,
                                             trigram_dct_size=trigram_dct_size,
                                             embedding_size=embedding_size,
                                             hidden_size=hidden_size,
                                             name=name,
                                             embedding_scope_name=embedding_scope_name)

        self.global_step = tf.get_variable('global_step',
                                            shape=[],
                                            initializer=tf.constant_initializer(0),
                                            trainable=False,
                                            dtype=tf.int32)

        self.learning_rate = learning_rate
        self.epsilon = epsilon
        self.grad_threshold = grad_threshold

        self.loss = self._calculate_crossentropy(self.hidden_states[:,:-1], self.word_ids[:,1:])
        self.next_word_distribution = self._get_next_word_distribution(self.final_hidden_state)

        self.train_op = self._optimize_loss()

        self.saver = tf.train.Saver(tf.global_variables(), max_to_keep=1)
        self.summary = tf.summary.scalar('loss', self.loss)

        print '--------------'
        print 'lm variables:'
        print [v.op.name for v in tf.global_variables()]
        print '--------------'


    def _calculate_crossentropy(self, hidden_states, word_ids):
        logits = tf.reshape(dense_layer('output_dense', tf.reshape(hidden_states, [-1, self.hidden_size]), self.word_dct_size),
                            [-1, self.max_num_words-1, self.word_dct_size])
        mask = tf.cast(tf.sequence_mask(self.num_words-1, self.max_num_words-1), tf.float32)
        return tf.reduce_mean(tf.reduce_sum(tf.nn.sparse_softmax_cross_entropy_with_logits(logits=logits, labels=word_ids) * mask, axis=1) / \
                              tf.cast(self.num_words-1, tf.float32))


    def _get_next_word_distribution(self, final_hidden_state):
        new_word_logits = dense_layer('output_dense', final_hidden_state, self.word_dct_size, reuse=True)
        return tf.nn.softmax(new_word_logits, dim=-1)


    def _optimize_loss(self):
        optimizer = tf.train.AdamOptimizer(self.learning_rate, epsilon=self.epsilon)

        var_list = tf.trainable_variables()
        grads = tf.gradients(self.loss, var_list)
        clipped_grads, _ = tf.clip_by_global_norm(grads, self.grad_threshold)

        return optimizer.apply_gradients(zip(clipped_grads, var_list), global_step=self.global_step)


    def get_next_word_distribution(self, sess, input):
        input_feed = self.get_input_feed(input)
        return sess.run(self.next_word_distribution, input_feed)


    def get_metrics_names(self):
        return ['loss']


    def __call__(self, sess, input, is_training=True):
        input_feed = self.get_input_feed(input)
        output_feed = [{'loss': self.loss}, self.summary]
        if is_training:
            output_feed.append(self.train_op)
        return sess.run(output_feed, input_feed)
