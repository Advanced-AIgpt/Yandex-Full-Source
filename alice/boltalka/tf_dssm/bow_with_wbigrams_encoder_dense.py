import tensorflow as tf
import numpy as np
from encoder_general import get_sparse_batch, dense_layer, get_batch


class BOW_with_wbigrams_encoder(object):
    def __init__(self,
                 word_dct_size,
                 trigram_dct_size,
                 wbigram_dct_size,
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
        self.wbigram_dct_size = wbigram_dct_size
        self.embedding_size = embedding_size
        self.hidden_size = hidden_size
        self.output_size = output_size

        self.word_ids = tf.placeholder(tf.int32, shape=[None,None], name='word_ids')
        self.num_words = tf.placeholder(tf.int32, shape=[None], name='num_words')
        self.trigram_ids = tf.sparse_placeholder(tf.int32, name='trigram_ids')
        self.trigram_batch_map = tf.placeholder(tf.int32, shape=[None], name='trigram_batch_map')
        self.wbigram_ids = tf.placeholder(tf.int32, shape=[None,None], name='wbigram_ids')
        self.num_wbigrams = tf.placeholder(tf.int32, shape=[None], name='num_wbigrams')

        self.num_wordtrigrams = tf.placeholder(tf.int32, shape=[None], name='num_wordtrigrams')
        self.max_num_wordtrigrams = tf.reduce_max(self.num_wordtrigrams)

        self.batch_size = tf.shape(self.word_ids)[0]

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

                # words
                word_embeddings_matrix = tf.get_variable('word_embeddings_matrix', dtype=tf.float32,
                                         initializer=tf.random_normal(shape=[self.word_dct_size, self.embedding_size], stddev=0.05))
                word_embeddings = tf.nn.embedding_lookup(word_embeddings_matrix, self.word_ids)
                word_embeddings_combined = self._combine_subtoken_embeddings_in_tokens(word_embeddings, self.num_words, pos_enc=True)

                # wordtrigrams
                trigram_embeddings_matrix = tf.get_variable('trigram_embeddings_matrix', dtype=tf.float32,
                                            initializer=tf.random_normal(shape=[self.trigram_dct_size, self.embedding_size], stddev=0.05))
                wordtrigram_embeddings_flat_base = tf.cond(tf.cast(self.trigram_batch_map[-1] != 0, tf.bool),
                                                           lambda: tf.concat(0, [tf.nn.embedding_lookup_sparse(trigram_embeddings_matrix, self.trigram_ids, None, combiner='sum'), tf.zeros([1, self.embedding_size])]),
                                                           lambda: tf.zeros([1, self.embedding_size]))
                wordtrigram_embeddings_flat = tf.gather(wordtrigram_embeddings_flat_base, self.trigram_batch_map)

                indices = tf.reshape(tf.tile(tf.range(self.max_num_wordtrigrams), [self.batch_size]), [self.batch_size, self.max_num_wordtrigrams]) + \
                          tf.expand_dims(tf.cumsum(self.num_wordtrigrams, exclusive=True), -1)
                indices = indices * tf.sequence_mask(self.num_wordtrigrams, self.max_num_wordtrigrams, dtype=tf.int32)

                wordtrigram_embeddings = tf.gather(wordtrigram_embeddings_flat, indices)
                wordtrigram_embeddings_combined = self._combine_subtoken_embeddings_in_tokens(wordtrigram_embeddings, self.num_wordtrigrams, pos_enc=True)

                # wbigrams
                wbigram_embeddings_matrix = tf.get_variable('wbigram_embeddings_matrix', dtype=tf.float32,
                                            initializer=tf.random_normal(shape=[self.wbigram_dct_size, self.embedding_size], stddev=0.05))
                wbigram_embeddings = tf.nn.embedding_lookup(wbigram_embeddings_matrix, self.wbigram_ids)
                wbigram_embeddings_combined = self._combine_subtoken_embeddings_in_tokens(wbigram_embeddings, self.num_wbigrams, pos_enc=True)

                b = tf.get_variable(name='b', shape=[self.embedding_size], dtype=tf.float32, initializer=tf.constant_initializer(0.0))

                return word_embeddings_combined + wordtrigram_embeddings_combined + wbigram_embeddings_combined + b


    def _combine_subtoken_embeddings_in_tokens(self, embeddings, num_tokens, pos_enc=False, epsilon=1e-20):
        embeddings = tf.cond(tf.cast(pos_enc, tf.bool),
                             lambda: embeddings * self._positional_encoding(num_tokens, tf.shape(embeddings)[1], epsilon),
                             lambda: embeddings)
        embeddings = embeddings * tf.expand_dims(tf.sequence_mask(num_tokens, tf.shape(embeddings)[1], dtype=tf.float32), -1)
        token_embeddings = tf.reduce_sum(embeddings, axis=1) / tf.expand_dims(tf.cast(num_tokens, tf.float32)+epsilon, -1)
        return token_embeddings


    def _positional_encoding(self, num_tokens, width, epsilon=1e-20):
        emb_pos = tf.cast(tf.range(self.embedding_size)+1, tf.float32) / float(self.embedding_size)
        emb_pos = tf.reshape(tf.tile(emb_pos, [self.batch_size]), [self.batch_size,1,-1])
        sent_pos = tf.cast(tf.reshape(tf.tile(tf.range(width)+1, [self.batch_size]), [self.batch_size, -1]), tf.float32) / \
                   tf.expand_dims(tf.cast(num_tokens, tf.float32)+epsilon, -1)
        sent_pos = tf.expand_dims(sent_pos, -1)
        batch_weights = tf.batch_matmul(1-sent_pos, 1-emb_pos) + 2*tf.batch_matmul(sent_pos, emb_pos)
        return batch_weights


    def _infer(self):
        sent_embeddings = self._get_embeddings()
        hidden = tf.nn.relu(dense_layer('hidden_dense', sent_embeddings, self.hidden_size))
        return dense_layer('output_dense', hidden, self.output_size)


    def get_input_feed(self, input):
        word_ids, num_words = get_batch(input[:,0])

        trigram_input = []
        num_wordtrigrams = []

        for line in input[:,1]:
            num_wordtrigrams.append(len(line))
            for subline in line:
                trigram_input.append(subline)

        trigram_ids, trigram_batch_map = get_sparse_batch(trigram_input, self.trigram_dct_size)
        wbigram_ids, num_wbigrams = get_batch(input[:,2])

        input_feed = {}
        input_feed[self.word_ids] = word_ids
        input_feed[self.num_words] = num_words
        input_feed[self.trigram_ids] = trigram_ids
        input_feed[self.trigram_batch_map] = trigram_batch_map
        input_feed[self.wbigram_ids] = wbigram_ids
        input_feed[self.num_wbigrams] = num_wbigrams

        input_feed[self.num_wordtrigrams] = num_wordtrigrams

        return input_feed


    def encode(self, sess, input):
        input_feed = self.get_input_feed(input)
        return sess.run(self.output, input_feed)
