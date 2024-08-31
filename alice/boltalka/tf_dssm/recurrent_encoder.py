import tensorflow as tf
import numpy as np
from encoder_general import get_batch, get_sparse_batch, dense_layer


class Recurrent_encoder(object):
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

        self.word_ids = tf.placeholder(tf.int32, shape=[None,None], name='word_ids')
        self.num_words = tf.placeholder(tf.int32, shape=[None], name='num_words')
        self.trigram_ids = tf.sparse_placeholder(tf.int32, name='trigram_ids')
        self.trigram_batch_map = tf.placeholder(tf.int32, shape=[None], name='trigram_batch_map')

        self.batch_size = tf.shape(self.word_ids)[0]
        self.max_num_words = tf.shape(self.word_ids)[1]

        with tf.variable_scope(name) as local_scope:
            self.hidden_states, self.final_hidden_state = self._infer()
            self.output = dense_layer('last_dense', self.final_hidden_state, output_size)
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
                word_embeddings = tf.nn.embedding_lookup(word_embeddings_matrix, self.word_ids)

                trigram_embeddings_matrix = tf.get_variable('trigram_embeddings_matrix', dtype=tf.float32,
                                            initializer=tf.random_normal(shape=[self.trigram_dct_size, self.embedding_size], stddev=0.05))
                wordtrigram_embeddings = self._combine_trigram_embeddings_in_words(trigram_embeddings_matrix, self.trigram_ids, self.trigram_batch_map, combiner='mean')
                embeddings = tf.concat(2, [word_embeddings, wordtrigram_embeddings])

                return embeddings


    def _combine_trigram_embeddings_in_words(self, embeddings_matrix, ids, batch_map, combiner):
        embeddings_sum_with_null_repr = tf.cond(tf.cast(batch_map[-1] != 0, tf.bool),
                                                lambda: tf.concat(0, [tf.nn.embedding_lookup_sparse(embeddings_matrix, ids, None, combiner=combiner),
                                                                      tf.zeros([1, self.embedding_size])]),
                                                lambda: tf.zeros([1, self.embedding_size]))
        embeddings_sum_in_batch_flat = tf.gather(embeddings_sum_with_null_repr, batch_map)

        indices = tf.reshape(tf.tile(tf.range(self.max_num_words), [self.batch_size]), [self.batch_size, self.max_num_words]) + \
                  tf.expand_dims(tf.cumsum(self.num_words, exclusive=True), -1)

        # critical for cpu (not gpu)
        indices = indices * tf.sequence_mask(self.num_words, self.max_num_words, dtype=tf.int32)

        return tf.gather(embeddings_sum_in_batch_flat, indices)


    def _infer(self):
        embeddings = self._get_embeddings()
        lstm_cell = tf.nn.rnn_cell.LSTMCell(self.hidden_size)

        hidden_states, (final_cell_state, final_hidden_state) = tf.nn.dynamic_rnn(lstm_cell,
                                                                                  embeddings,
                                                                                  dtype=tf.float32,
                                                                                  sequence_length=self.num_words)

        return hidden_states, final_hidden_state


    def get_input_feed(self, input):
        word_ids, num_words = get_batch(input[:,0])
        trigram_ids, trigram_batch_map = get_sparse_batch([subline for line in input[:,1] for subline in line], self.trigram_dct_size)
        input_feed = {}
        input_feed[self.word_ids] = word_ids
        input_feed[self.num_words] = num_words
        input_feed[self.trigram_ids] = trigram_ids
        input_feed[self.trigram_batch_map] = trigram_batch_map
        return input_feed


    def encode(self, sess, input):
        input_feed = self.get_input_feed(input)
        return sess.run(self.output, input_feed)
