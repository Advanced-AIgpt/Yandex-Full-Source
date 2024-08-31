import tensorflow as tf
import numpy as np
import codecs


class DSSM(object):
    def __init__(self,
                 encoders_configs,
                 loss_maker,
                 learning_rate,
                 epsilon,
                 grad_threshold,
                 num_negatives,
                 triplet_loss_threshold,
                 left_anchor_prob,
                 are_there_labels):

        self.scope = tf.get_variable_scope()
        self.learning_rate = learning_rate
        self.epsilon = epsilon
        self.grad_threshold = grad_threshold
        self.num_negatives = num_negatives
        self.triplet_loss_threshold = triplet_loss_threshold
        self.left_anchor_prob = left_anchor_prob
        self.are_there_labels = are_there_labels

        self.ns_placeholder = tf.placeholder(tf.int32, shape=[None, num_negatives], name='ns')
        self.anchors_placeholder = tf.placeholder(tf.bool, shape=[None], name='anchors')

        self.global_step = tf.get_variable('global_step',
                                           shape=[],
                                           initializer=tf.constant_initializer(0),
                                           trainable=False,
                                           dtype=tf.int32)

        self.encoders = self._encode(encoders_configs)
        self.triplet_loss, self.recall = loss_maker(self.encoders[0].output,
                                                    self.encoders[1].output,
                                                    triplet_loss_threshold,
                                                    self.ns_placeholder, self.anchors_placeholder)
        self.train_op = self._optimize_loss(self.triplet_loss)

        self.saver = tf.train.Saver(tf.global_variables(), max_to_keep=1)

        tf.summary.scalar('loss', self.triplet_loss)
        tf.summary.scalar('recall', self.recall)
        self.merged_summary = tf.summary.merge_all()

        print '--------------'
        print 'dssm variables:'
        print [v.op.name for v in tf.global_variables()]
        print '--------------'


    def _encode(self, encoders_configs):
        encoders = []
        for encoder_config in encoders_configs:
            encoders.append(encoder_config['model'](name=encoder_config['name'],
                                                    outside_scope=self.scope,
                                                    **encoder_config['params']))
        return encoders


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


    def _sample_negative_ids(self, batch_size, num_negatives):
        return np.array([np.random.choice(np.delete(np.arange(batch_size), i), size=num_negatives, replace=False) for i in xrange(batch_size)])


    def _is_anchor_left(self, batch_size, p):
        return np.random.uniform(0.0, 1.0, batch_size) <= p


    def get_metrics_names(self):
        return ['loss', 'recall']


    def _get_input_feed(self, input):
        if self.are_there_labels:
            labels = input[:,-1]
            input = np.array([list(x) for x in input[:,:-1]])

        replies = input[:,-1]
        contexts = input[:,:-1]
        if contexts.shape[1] == 1:
            contexts = contexts[:,0]

        if self.are_there_labels:
            positives_mask = labels == 1
            assert len(positives_mask) % positives_mask.sum() == 0
            replies = np.vstack([replies[positives_mask], replies[~positives_mask]])
            contexts = contexts[positives_mask]

        input_feed_right_encoder = self.encoders[1].get_input_feed(replies)
        input_feed_left_encoder = self.encoders[0].get_input_feed(contexts)

        input_feed_other = {self.ns_placeholder: self._sample_negative_ids(len(contexts), self.num_negatives),
                            self.anchors_placeholder: self._is_anchor_left(len(contexts), self.left_anchor_prob)}

        input_feed = {}
        for dct in [input_feed_left_encoder, input_feed_right_encoder, input_feed_other]:
            input_feed.update(dct)

        return input_feed


    def __call__(self, sess, input, is_training=True):
        input_feed = self._get_input_feed(input)

        output_feed = [{'loss': self.triplet_loss, 'recall': self.recall}, self.merged_summary]
        if is_training:
            output_feed.append(self.train_op)

        return sess.run(output_feed, input_feed)
