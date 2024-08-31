import tensorflow as tf
import logging

from vins_core.nlu.neural.metric_learning.encoders.stack_sequential import RecurrentStackSequentialEncoder
from vins_core.nlu.neural.metric_learning.losses import get_loss


logger = logging.getLogger(__name__)


class MetricLearningModel(object):

    def __init__(self, config, preprocessor, create_for_train):
        self.config = config
        self.preprocessor = preprocessor
        self._create_for_train = create_for_train
        self._encoder_applier = None

        self._init_placeholders()
        self._init_encoders()

        if self._create_for_train:
            self._init_loss()
            self._init_grads()

    def _init_placeholders(self):
        self.placeholders = {
            'labels': tf.placeholder(tf.int32, shape=[None])
        }

    def _init_grads(self):
        var_list1, var_list2 = [], []
        for v in tf.trainable_variables():
            name = v.op.name
            if name.endswith('embeddings_matrix'):
                var_list1.append(v)
            else:
                var_list2.append(v)

        grads = tf.gradients(self.loss_maker.output_feed[self.loss_maker.get_loss_name()],
                             var_list1 + var_list2)
        self.grads, _ = tf.clip_by_global_norm(grads, self.config.grad_threshold)

    def _init_loss(self):
        loss_params = self.config.loss_params

        self.loss_maker = get_loss(loss_params['name'])(
            samples=self.encoder.output,
            precomputed_negatives_mask=self.preprocessor.masker.get_precomputed_negatives_mask(),
            precomputed_anchors_mask=self.preprocessor.masker.get_precomputed_anchors_mask(),
            precomputed_recall_mask=self.preprocessor.masker.get_precomputed_recall_mask(),
            labels=self.placeholders['labels'],
            **loss_params
        )

    def _init_encoders(self):
        self.encoder = RecurrentStackSequentialEncoder(self.config, self._create_for_train,
                                                       self.preprocessor.dense_seq_embeddings,
                                                       self.preprocessor.input_sizes_map)

    def get_input_feed(self, inputs, is_training):
        input_feed = {placeholder: inputs[key] for key, placeholder in self.placeholders.iteritems()}
        input_feed.update(self.encoder.get_input_feed(inputs['context'], is_training))

        return input_feed

    def make_applier(self, sess):
        self._encoder_applier = self.encoder.convert_to_applier(sess)

    def encode(self, sess, input):
        if self._encoder_applier is not None:
            return self._encoder_applier.encode(input)
        return self.encoder.encode(sess, input)
