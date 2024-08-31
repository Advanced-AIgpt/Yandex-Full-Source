import tensorflow as tf
import numpy as np
from encoder_general import dense_layer


class Complex_encoder(object):
    def __init__(self,
                 encoders_configs,
                 output_size=300,
                 name='',
                 outside_scope=tf.get_variable_scope()):

        self.name = name
        self.outside_scope = outside_scope
        self.output_size = output_size

        with tf.variable_scope(name) as local_scope:
            self.encoders = self._get_encoders(encoders_configs)
            self.output = self._make_output()
            self.output = tf.nn.l2_normalize(self.output, dim=1)

        saver_prefixes = [local_scope.name] + [encoder.embedding_scope_name for encoder in self.encoders]
        self.saver = tf.train.Saver(sum([tf.get_collection(tf.GraphKeys.GLOBAL_VARIABLES, scope=prefix)
                                         for prefix in saver_prefixes], []), max_to_keep=1)


    def _get_encoders(self, encoders_configs):
        encoders = []
        for encoder_config in encoders_configs:
            encoders.append(encoder_config['model'](name=encoder_config['name'],
                                                    outside_scope=self.outside_scope,
                                                    **encoder_config['params']))
        return encoders


    def _make_output(self):
        output = tf.concat(1, [encoder.output for encoder in self.encoders])
        return dense_layer('complex_dense', output, self.output_size)


    def get_input_feed(self, input):
        input_feed = []
        for idx in xrange(len(self.encoders)):
            input_feed.extend(self.encoders[idx].get_input_feed(input[:,idx]).items())
        input_feed = dict(input_feed)
        return input_feed


    def encode(self, sess, input):
        input_feed = self.get_input_feed(input)
        return sess.run(self.output, input_feed)
