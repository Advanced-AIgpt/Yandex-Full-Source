import tensorflow as tf


def _select_elements_by_column_indices(tensor, column_indices):
    num_rows = tf.shape(tensor)[0]
    num_col_ids = tf.shape(column_indices)[1]
    flat_cols = tf.reshape(column_indices, [-1])
    flat_rows = tf.reshape(tf.tile(tf.expand_dims(tf.range(num_rows, dtype=tf.int32), -1), (1, num_col_ids)), [-1])
    ij = tf.stack((flat_rows, flat_cols), axis=-1)
    return tf.reshape(tf.gather_nd(tensor, ij), (num_rows, -1))


class BaseLoss(object):

    _INF = 1e9

    def __init__(self, samples, labels, precomputed_negatives_mask, precomputed_anchors_mask=None,
                 precomputed_recall_mask=None, positive_mining='hard', num_positives=1,
                 negative_mining='semihard', num_negatives=1, trash_non_anchors=True, **kwargs):
        MASK_VAR_PARAMS = {'trainable': False, 'collections': [tf.GraphKeys.LOCAL_VARIABLES]}

        if precomputed_recall_mask is None:
            recall_mask = None
        else:
            recall_mask = tf.gather(tf.Variable(precomputed_recall_mask, name='recall_mask',
                                                dtype=tf.bool, **MASK_VAR_PARAMS), labels)

        if precomputed_anchors_mask is None:
            anchors_mask = None
        else:
            anchors_mask = tf.gather(tf.Variable(precomputed_anchors_mask, name='anchors_mask',
                                                 dtype=tf.bool, **MASK_VAR_PARAMS), labels)

        pnm_var = tf.Variable(precomputed_negatives_mask, name='precomputed_negatives_mask',
                              dtype=tf.float32, **MASK_VAR_PARAMS)
        precomputed_negatives_mask = tf.transpose(tf.gather(tf.transpose(tf.gather(pnm_var, labels)), labels))

        positives_mask = tf.cast(tf.equal(tf.expand_dims(labels, axis=1), tf.expand_dims(labels, axis=0)), tf.float32)
        negatives_mask = tf.multiply(1. - positives_mask, precomputed_negatives_mask)

        self._samples = samples
        self._diag_mask = 1 - tf.eye(tf.shape(samples)[0])

        self._anchors_mask = anchors_mask
        self._trash_non_anchors = trash_non_anchors

        if anchors_mask is None or not trash_non_anchors:
            self._anchors = self._samples
            self._positives_mask = tf.cast(positives_mask, tf.float32)
            self._negatives_mask = tf.cast(negatives_mask, tf.float32)
            self._labels = labels
            self._recall_mask = recall_mask
        else:
            self._anchors = tf.boolean_mask(self._samples, anchors_mask)
            self._positives_mask = tf.cast(tf.boolean_mask(positives_mask, anchors_mask), tf.float32)
            self._negatives_mask = tf.cast(tf.boolean_mask(negatives_mask, anchors_mask), tf.float32)
            self._labels = tf.boolean_mask(labels, anchors_mask)
            self._diag_mask = tf.boolean_mask(self._diag_mask, anchors_mask)
            if recall_mask is not None:
                self._recall_mask = tf.boolean_mask(recall_mask, anchors_mask)
            else:
                self._recall_mask = recall_mask

        self._num_anchors = tf.shape(self._anchors)[0]
        self._positive_mining = positive_mining
        self._negative_mining = negative_mining
        self._num_positives = num_positives
        self._num_negatives = num_negatives

        self._sims = tf.matmul(a=self._anchors, b=self._samples, transpose_b=True)

        self.output_feed = {}
        with tf.variable_scope(name_or_scope=self.get_loss_name()):
            self.output_feed[self.get_loss_name()] = self._get_loss()
        with tf.variable_scope(name_or_scope='recalls'):
            self.output_feed['recall'] = self._get_recall(positive_mining='hard')
            self.output_feed['uniform_recall'] = self._get_recall(positive_mining='uniform')

    def _select_sims(self, sims, mask, diag_mask, mining, num_samples, anchor_sims=None):
        mask = tf.multiply(mask, diag_mask)
        if mining == 'uniform':
            ids = tf.cast(tf.multinomial(tf.log(mask), num_samples), tf.int32)
        elif mining == 'hard':
            _, ids = tf.nn.top_k(sims - self._INF * (1 - mask), k=num_samples)
        elif mining == 'semihard':
            if anchor_sims is None:
                raise ValueError('semihard mining is specified by anchor similarities are not presented')
            semihard_mask = tf.cast(tf.less(sims, tf.expand_dims(anchor_sims[:, -1], -1)), tf.float32)
            _, ids = tf.nn.top_k(sims - self._INF * (1 - semihard_mask * mask), k=num_samples)
        else:
            raise NotImplementedError('%s mining is not implemented' % mining)
        return _select_elements_by_column_indices(sims, ids)

    def _get_sims_for_loss(self):
        pos_sims = self._select_sims(
            self._sims, self._positives_mask, self._diag_mask, self._positive_mining, self._num_positives)
        neg_sims = self._select_sims(
            self._sims, self._negatives_mask, self._diag_mask, self._negative_mining, self._num_negatives,
            anchor_sims=pos_sims)
        return pos_sims, neg_sims

    def _get_sims_for_recall(self, positive_mining='hard'):
        pos_sims = self._select_sims(self._sims, self._positives_mask, self._diag_mask, mining=positive_mining,
                                     num_samples=1)
        neg_sims = self._select_sims(self._sims, self._negatives_mask, self._diag_mask, mining='hard', num_samples=1)
        if self._recall_mask is not None:
            pos_sims = tf.boolean_mask(pos_sims, self._recall_mask)
            neg_sims = tf.boolean_mask(neg_sims, self._recall_mask)
        return pos_sims, neg_sims

    def _get_loss(self):
        pos_sims, neg_sims = self._get_sims_for_loss()
        self._create_summaries(pos_sims, neg_sims)
        return self.get_loss(pos_sims, neg_sims)

    def _get_recall(self, at=1, positive_mining='hard'):
        pos_sims, neg_sims = self._get_sims_for_recall(positive_mining)
        cnt_neg_win = tf.reduce_sum(tf.cast(tf.greater_equal(neg_sims, tf.reshape(pos_sims, (-1, 1))), tf.float32), 1)
        self._create_summaries(pos_sims, neg_sims, histogram=True)
        return tf.reduce_mean(tf.cast(tf.less(cnt_neg_win, at), tf.float32))

    def get_loss(self, pos_sims, neg_sims):
        raise NotImplementedError

    @staticmethod
    def get_loss_name():
        return 'loss'

    def _create_summaries(self, pos_sims, neg_sims, histogram=False):
        tf.summary.scalar('mean_pos_sims', tf.reduce_mean(pos_sims))
        tf.summary.scalar('mean_neg_sims', tf.reduce_mean(neg_sims))
        if histogram:
            tf.summary.histogram('pos_sim', pos_sims)
            tf.summary.histogram('neg_sim', neg_sims)


class TripletLoss(BaseLoss):

    def __init__(self, margin=0.1, **kwargs):
        self._margin = margin
        super(TripletLoss, self).__init__(**kwargs)

    def get_loss(self, pos_sims, neg_sims):
        return tf.reduce_mean(tf.maximum(0.0, neg_sims - pos_sims + self._margin))


class MarginLoss(BaseLoss):

    def __init__(self, threshold=0.0, margin=0.1, trainable_threshold=True, threshold_regularizer=0, **kwargs):
        self._trainable_threshold = trainable_threshold
        self._threshold = tf.get_variable(
            'threshold', trainable=trainable_threshold, initializer=tf.constant(float(threshold)), dtype=tf.float32
        )
        self._margin = margin
        self._threshold_regularizer = threshold_regularizer

        super(MarginLoss, self).__init__(**kwargs)

    def get_loss(self, pos_sims, neg_sims):
        pos_loss = tf.maximum(self._threshold - pos_sims + self._margin, 0.0)
        neg_loss = tf.maximum(neg_sims - self._threshold + self._margin, 0.0)

        if not self._trash_non_anchors:
            pos_loss = tf.multiply(pos_loss, tf.cast(self._anchors_mask, tf.float32))

        return tf.reduce_mean(pos_loss + neg_loss) + self._threshold_regularizer * self._threshold

    def _create_summaries(self, pos_sims, neg_sims, histogram=False):
        super(MarginLoss, self)._create_summaries(pos_sims, neg_sims, histogram)
        fr = tf.reduce_sum(tf.cast(tf.less(pos_sims, self._threshold), tf.float32))
        fa = tf.reduce_sum(tf.cast(tf.greater(neg_sims, self._threshold), tf.float32))
        if self._trainable_threshold:
            tf.summary.scalar('threshold', self._threshold)
        tf.summary.scalar('false_rejects', fr)
        tf.summary.scalar('false_alarms', fa)
        tf.summary.scalar('fa_minus_fr', fa - fr)


class LogLoss(BaseLoss):

    def __init__(self, scale=1.0, threshold=0.1, trainable_scale=True, trainable_threshold=True, **kwargs):
        self._trainable_scale = trainable_scale
        self._trainable_threshold = trainable_threshold
        self._threshold = tf.get_variable(
            'threshold', trainable=trainable_threshold, initializer=tf.constant(float(threshold)), dtype=tf.float32)
        self._scale = tf.get_variable(
            'scale', trainable=trainable_scale, initializer=tf.constant(float(scale)), dtype=tf.float32)
        super(LogLoss, self).__init__(**kwargs)

    def get_loss(self, pos_sims, neg_sims):
        return tf.reduce_mean(
            tf.log(1 + tf.exp(self._threshold - self._scale * pos_sims)) +
            tf.log(1 + tf.exp(self._scale * neg_sims - self._threshold))
        )

    def _create_summaries(self, pos_sims, neg_sims, histogram=True):
        super(LogLoss, self)._create_summaries(pos_sims, neg_sims, histogram)
        fr = tf.reduce_sum(tf.cast(tf.less_equal(pos_sims, self._threshold), tf.float32))
        fa = tf.reduce_sum(tf.cast(tf.greater_equal(neg_sims, self._threshold), tf.float32))
        if self._trainable_threshold:
            tf.summary.scalar('threshold', self._threshold)
        if self._trainable_scale:
            tf.summary.scalar('scale', self._scale)
        tf.summary.scalar('false_rejects', fr)
        tf.summary.scalar('false_alarms', fa)
        tf.summary.scalar('fa_minus_fr', fa - fr)


class TopKSoftmaxLoss(BaseLoss):

    def __init__(self, top_k=10, threshold=0.0, scale=1.0, trainable_scale=True, trainable_threshold=True, **kwargs):
        self._top_k = top_k
        self._trainable_scale = trainable_scale
        self._trainable_threshold = trainable_threshold
        self._threshold = tf.get_variable(
            'threshold', trainable=trainable_threshold, initializer=tf.constant(float(threshold)), dtype=tf.float32)
        self._scale = tf.get_variable(
            'scale', trainable=trainable_scale, initializer=tf.constant(float(scale)), dtype=tf.float32)
        super(TopKSoftmaxLoss, self).__init__(**kwargs)

    def get_loss(self, pos_sims, neg_sims):
        num_pos = tf.shape(pos_sims)[1]
        sims = tf.concat((pos_sims, neg_sims), axis=1)
        log_softmax = tf.nn.log_softmax(self._scale * sims + self._threshold)[:, :num_pos]
        return -tf.reduce_mean(log_softmax)

    def _create_summaries(self, pos_sims, neg_sims, histogram=False):
        super(TopKSoftmaxLoss, self)._create_summaries(pos_sims, neg_sims, histogram)
        if self._trainable_threshold:
            tf.summary.scalar('threshold', self._threshold)
        if self._trainable_scale:
            tf.summary.scalar('scale', self._scale)


_losses_registry = {}


def register_loss(loss_type, name):
    assert issubclass(loss_type, BaseLoss)
    _losses_registry[name] = loss_type


def get_loss(name):
    if name not in _losses_registry:
        raise ValueError('Unsupported loss type %s' % name)

    return _losses_registry[name]


register_loss(TripletLoss, 'triplet')
register_loss(MarginLoss, 'margin')
register_loss(LogLoss, 'logloss')
register_loss(TopKSoftmaxLoss, 'topksoftmax')
