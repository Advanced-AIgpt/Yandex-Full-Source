import tensorflow as tf
import numpy as np
import codecs
from encoder_general import dense_layer


class Classifier(object):
    def __init__(self,
                 encoder_config,
                 learning_rate,
                 epsilon,
                 grad_threshold):

        self.scope = tf.get_variable_scope()
        self.learning_rate = learning_rate
        self.epsilon = epsilon
        self.grad_threshold = grad_threshold

        self.global_step = tf.get_variable('global_step',
                                           shape=[],
                                           initializer=tf.constant_initializer(0),
                                           trainable=False,
                                           dtype=tf.int32)

        self.encoder = encoder_config['model'](name=encoder_config['name'],
                                               outside_scope=self.scope,
                                               **encoder_config['params'])

        self.output = self.encoder.output
        self.logits = dense_layer('output_dense', self.output, 2)

        self.labels = tf.placeholder(tf.int32, [None], name='labels')

        self.loss, self.accuracy = self._calculate_metrics()

        self.train_op = self._optimize_loss(self.loss)

        self.saver = tf.train.Saver(tf.global_variables(), max_to_keep=1)

        tf.summary.scalar('loss', self.loss)
        tf.summary.scalar('accuracy', self.accuracy)
        self.merged_summary = tf.summary.merge_all()

        print '--------------'
        print 'classifier variables:'
        print [v.op.name for v in tf.global_variables()]
        print '--------------'


    def _calculate_metrics(self):
        loss = tf.reduce_mean(tf.nn.sparse_softmax_cross_entropy_with_logits(logits=self.logits, labels=self.labels))
        accuracy = tf.reduce_mean(tf.cast(tf.equal(tf.cast(tf.argmax(self.logits, axis=1), tf.int32), self.labels), tf.float32))
        return loss, accuracy


    def _optimize_loss(self, loss):
        var_list1, var_list2 = [], []
        for v in tf.trainable_variables():
            name = v.op.name
            if name.endswith('embeddings_matrix'):
                var_list1.append(v)
            else:
                var_list2.append(v)

        optimizer1 = tf.train.MomentumOptimizer(64.0, 0.0)
        optimizer2 = tf.train.AdamOptimizer(self.learning_rate, epsilon=self.epsilon)

        grads = tf.gradients(loss, var_list1 + var_list2)
        grads, _ = tf.clip_by_global_norm(grads, self.grad_threshold)
        grads1 = grads[:len(var_list1)]
        grads2 = grads[len(var_list1):]

        train_op1 = optimizer1.apply_gradients(zip(grads1, var_list1),
                                               global_step=self.global_step)
        train_op2 = optimizer2.apply_gradients(zip(grads2, var_list2))
        return tf.group(train_op1, train_op2)


    def get_metrics_names(self):
        return ['loss', 'accuracy']


    def predict(self, sess, input):
        input_feed = self._get_encoder_input_feed(input)
        return sess.run(tf.nn.softmax(self.logits)[:,1], input_feed)


    def _get_labels(self, input):
        return {self.labels: input}


    def _get_encoder_input_feed(self, input):
        return self.encoder.get_input_feed(input)


    def __call__(self, sess, input, is_training=True):
        text_data = self._get_encoder_input_feed(np.array(list(input[:,0]), dtype=object))
        labels = self._get_labels(input[:,1])

        input_feed = {}
        for dct in [text_data, labels]:
            input_feed.update(dct)

        output_feed = [{'loss': self.loss, 'accuracy': self.accuracy}, self.merged_summary]
        if is_training:
            output_feed.append(self.train_op)

        return sess.run(output_feed, input_feed)
