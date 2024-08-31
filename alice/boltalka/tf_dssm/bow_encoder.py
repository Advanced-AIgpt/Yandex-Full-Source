import tensorflow as tf
import numpy as np
from encoder_general import get_sparse_batch, dense_layer


class BOW_encoder(object):
    def __init__(self,
                 word_dct_size,
                 trigram_dct_size,
                 embedding_size=300,
                 hidden_size=300,
                 output_size=300,
                 name='',
                 outside_scope=tf.get_variable_scope(),
                 embedding_scope_name='embeddings'):

        self.name = name
        self.outside_scope = outside_scope
        self.embedding_scope_name = embedding_scope_name

        self.word_dct_size = word_dct_size
        self.trigram_dct_size = trigram_dct_size
        self.embedding_size = embedding_size
        self.hidden_size = hidden_size
        self.output_size = output_size

        self.word_ids = tf.sparse_placeholder(tf.int32, name='word_ids')
        self.word_batch_map = tf.placeholder(tf.int32, shape=[None], name='word_batch_map')
        self.trigram_ids = tf.sparse_placeholder(tf.int32, name='trigram_ids')
        self.trigram_batch_map = tf.placeholder(tf.int32, shape=[None], name='trigram_batch_map')

        with tf.variable_scope(name) as local_scope:
            self.output = self._infer()
            self.output = tf.nn.l2_normalize(self.output, dim=1)

        self.saver = tf.train.Saver(tf.get_collection(tf.GraphKeys.GLOBAL_VARIABLES, scope=local_scope.name)+\
                                    tf.get_collection(tf.GraphKeys.GLOBAL_VARIABLES, scope=self.embedding_scope_name),
                                    max_to_keep=1)


    def _get_embeddings(self):
        with tf.variable_scope(self.outside_scope):
            with tf.variable_scope(self.embedding_scope_name,
                        reuse=len(tf.get_collection(tf.GraphKeys.GLOBAL_VARIABLES, scope=self.embedding_scope_name)) != 0):
                word_embeddings_matrix = tf.get_variable('word_embeddings_matrix', dtype=tf.float32,
                                         initializer=tf.random_normal(shape=[self.word_dct_size, self.embedding_size], stddev=0.05))
                word_embeddings_sum = self._combine_embeddings(word_embeddings_matrix, self.word_ids, self.word_batch_map, combiner='mean')

                trigram_embeddings_matrix = tf.get_variable('trigram_embeddings_matrix', dtype=tf.float32,
                                            initializer=tf.random_normal(shape=[self.trigram_dct_size, self.embedding_size], stddev=0.05))
                trigram_embeddings_sum = self._combine_embeddings(trigram_embeddings_matrix, self.trigram_ids, self.trigram_batch_map, combiner='mean')

                b = tf.get_variable(name='b', shape=[self.embedding_size], dtype=tf.float32, initializer=tf.constant_initializer(0.0))

                return word_embeddings_sum + trigram_embeddings_sum + b


    def _combine_embeddings(self, embeddings_matrix, ids, batch_map, combiner):
        embeddings_sum_with_null_repr = tf.cond(tf.cast(batch_map[-1] != 0, tf.bool),
                                                lambda: tf.concat(0, [tf.nn.embedding_lookup_sparse(embeddings_matrix, ids, None, combiner=combiner), tf.zeros([1, self.embedding_size])]),
                                                lambda: tf.zeros([1, self.embedding_size]))
        return tf.gather(embeddings_sum_with_null_repr, batch_map)


    def _infer(self):
        sent_embeddings = self._get_embeddings()
        hidden = tf.nn.relu(dense_layer('hidden_dense', sent_embeddings, self.hidden_size))
        return dense_layer('output_dense', hidden, self.output_size)


    def get_input_feed(self, input):
        word_ids, word_batch_map = get_sparse_batch(input[:,0], self.word_dct_size)
        trigram_ids, trigram_batch_map = get_sparse_batch(input[:,1], self.trigram_dct_size)

        input_feed = {}
        input_feed[self.word_ids] = word_ids
        input_feed[self.word_batch_map] = word_batch_map
        input_feed[self.trigram_ids] = trigram_ids
        input_feed[self.trigram_batch_map] = trigram_batch_map

        return input_feed


    def encode(self, sess, input):
        input_feed = self.get_input_feed(input)
        return sess.run(self.output, input_feed)
