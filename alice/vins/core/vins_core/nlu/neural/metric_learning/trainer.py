# -*- coding: utf-8 -*-
import logging
import time
import tensorflow as tf
import os
from collections import Counter
from copy import copy

from tensorflow.core.util.event_pb2 import SessionLog
from tensorflow.contrib.framework.python.framework import list_variables


logger = logging.getLogger(__name__)


class SaveController(object):
    def __init__(self, metrics_names):
        self.signs = dict()
        for m in metrics_names:
            if 'loss' in m:
                self.signs[m] = -1
            elif 'recall' in m:
                self.signs[m] = 1
            else:
                logger.warning('Cannot determine type of %s. It will be ignored', m)
        self.best_saved_scores = None
        self.metrics_names = metrics_names

    def are_scores_improved(self, new_scores):
        '''If at least one score shows an improvement, save'''
        if self.best_saved_scores is None:
            self.best_saved_scores = copy(new_scores)
            return True
        is_improved = False
        for m in self.metrics_names:
            if (new_scores[m] - self.best_saved_scores[m]) * self.signs[m] > 0:
                logger.info('%s is improved: %f --> %f', m, self.best_saved_scores[m], new_scores[m])
                self.best_saved_scores[m] = new_scores[m]
                is_improved = True
        if not is_improved:
            logger.info('Not an improvement, best scores: %r', self.best_saved_scores)
        return is_improved


def create_summary(dic):
    return tf.Summary(value=[tf.Summary.Value(tag=name, simple_value=value)
                             for name, value in dic.iteritems()])


def get_checkpoint_path(restore_path, mode='last'):
    if os.path.isdir(restore_path):
        ckpt = tf.train.get_checkpoint_state(restore_path)
        if ckpt and ckpt.model_checkpoint_path:
            all_paths = list(ckpt.all_model_checkpoint_paths)
            if mode == 'last':
                checkpoint_path = ckpt.model_checkpoint_path
            elif mode.startswith('checkpoint'):
                checkpoint_path = next((path for path in all_paths if mode in path), None)
                if not checkpoint_path:
                    raise ValueError('Can''t find checkpoint matching mode=%s' % mode)
            else:
                raise ValueError('Unknown checkpoint path mode %s' % mode)
        else:
            logger.warning('Nothing to restore from directory %s', restore_path)
            return None
    elif os.path.isfile(restore_path + '.meta'):
        checkpoint_path = restore_path
    else:
        logger.warning('Nothing to restore from %s', restore_path)
        return None
    return checkpoint_path


class Applier(object):
    def __init__(self, config, save_config, preprocessor, create_for_train=False):
        self._graph = tf.Graph()
        self._config = config
        self._save_config = save_config
        self._preprocessor = preprocessor

        with self._graph.as_default(), tf.device('/cpu:0'):
            self._init_tf_config()

            self.global_step = tf.get_variable(name='global_step', shape=[], initializer=tf.constant_initializer(0),
                                               trainable=False, dtype=tf.int32)

            with self._graph.device('/gpu:0' if create_for_train else '/cpu:0'):
                with tf.name_scope('gpu0'):
                    self.model = self._config.model(self._config, self._preprocessor, create_for_train)

            self._init_graph()

            self._init_savers()

            self.sess = tf.Session(config=self.tfconfig)
            self.sess.run(tf.global_variables_initializer())
            self.sess.run(tf.local_variables_initializer())

    def _init_graph(self):
        pass

    def _init_tf_config(self):
        self.tfconfig = tf.ConfigProto()
        self.tfconfig.allow_soft_placement = True
        self.tfconfig.log_device_placement = False
        self.tfconfig.gpu_options.allow_growth = True

    def get_checkpoint_path(self):
        return os.path.join(self._save_config['checkpoint_folder'], self._config.run_name)

    def _init_savers(self):
        if not os.path.exists(self.get_checkpoint_path()):
            os.makedirs(self.get_checkpoint_path())

        if not os.path.exists(os.path.join(self.get_checkpoint_path(), self._config.checkpoint_file)):
            var_list = tf.global_variables()
        else:
            local_vars = self._get_all_local_variables()
            checkpoint_vars = self._get_all_checkpoint_variables()
            avaliable_vars = set(local_vars.keys()) & set(checkpoint_vars.keys())
            extra_checkpoints_vars = set(checkpoint_vars.keys()) - avaliable_vars
            extra_local_vars = set(local_vars.keys()) - avaliable_vars
            if len(extra_checkpoints_vars) > 0:
                logger.warning(
                    'Extra checkpoint vars: %s/%s' % (len(extra_checkpoints_vars), len(checkpoint_vars)))
                logger.warning('\n'.join(sorted(extra_checkpoints_vars)))
            else:
                logger.info('All vars from checkpoint used')
            if len(extra_local_vars) > 0:
                logger.warning('Extra local vars: %s/%s' % (len(extra_local_vars), len(local_vars)))
                logger.warning('\n'.join(sorted(extra_local_vars)))
            else:
                logger.info('All vars from environment used')
            logger.info('Using %s/%s vars' % (len(avaliable_vars), len(local_vars)))
            var_list = set(local_vars[var] for var in avaliable_vars)

        self.saver = tf.train.Saver(
            var_list=var_list,
            max_to_keep=self._config.max_num_checkpoints,
            save_relative_paths=True
        )

    def _get_all_local_variables(self):
        return {var.name[:-2]: var for var in tf.global_variables()}

    def _get_all_checkpoint_variables(self):
        lst = list_variables(self.get_checkpoint_path())
        return {unicode(k): k for k, v in lst}

    def make_applier(self):
        self.model.make_applier(self.sess)

    def encode(self, batch):
        data = self._preprocessor.extract_data(batch, for_train=False)

        return self.model.encode(self.sess, data)

    @property
    def step(self):
        return self.global_step.eval(session=self.sess)

    def restore_weights(self, path=None, mode='last'):
        if path is None:
            path = self.get_checkpoint_path()

        checkpoint_path = get_checkpoint_path(path, mode)
        if checkpoint_path is None:
            return False

        self.saver.restore(self.sess, checkpoint_path)
        first_step = self.step
        logger.info('Restored checkpoint path %s, first step = %d', checkpoint_path, first_step)

        return True


class Trainer(Applier):
    def __init__(self, config, save_config, preprocessor):
        logger.info('Creating Trainer')

        super(Trainer, self).__init__(config, save_config, preprocessor, create_for_train=True)

    def _init_graph(self):
        self.output_feed_total = self.model.loss_maker.output_feed

        self.train_op = self.apply_grads(self.model.grads)

        self._init_loggers()

        self.validation_metrics = None

    def apply_grads(self, grads):
        var_list1, var_list2 = [], []
        for v in tf.trainable_variables():
            name = v.op.name
            if name.endswith('embeddings_matrix'):
                var_list1.append(v)
            else:
                var_list2.append(v)

        if self._config.learning_rate_decay_rate is not None:
            learning_rate = tf.train.exponential_decay(
                learning_rate=self._config.learning_rate,
                global_step=self.global_step,
                decay_steps=self._config.learning_rate_decay_steps,
                staircase=True,
                decay_rate=self._config.learning_rate_decay_rate)
        else:
            learning_rate = self._config.learning_rate

        tf.summary.scalar('learning_rate', learning_rate)

        optimizer2 = tf.train.AdamOptimizer(learning_rate, epsilon=self._config.epsilon)
        train_op = optimizer2.apply_gradients(grads_and_vars=zip(grads[len(var_list1):], var_list2),
                                              global_step=self.global_step)

        if len(var_list1) > 0:
            if self._config.embedding_learning_rate > 0:
                optimizer1 = tf.train.MomentumOptimizer(self._config.embedding_learning_rate, 0.0)
            else:
                optimizer1 = tf.train.AdamOptimizer(learning_rate, epsilon=self._config.epsilon)

            train_op1 = optimizer1.apply_gradients(grads_and_vars=zip(grads[:len(var_list1)], var_list1),
                                                   global_step=self.global_step)
            train_op = tf.group(train_op1, train_op)

        return train_op

    def get_metrics_path(self):
        return os.path.join(self._save_config['logdir'], self._config.run_name)

    def _init_loggers(self):
        if not os.path.exists(self.get_metrics_path()):
            os.makedirs(self.get_metrics_path())

        for name, tensor in self.output_feed_total.items():
            tf.summary.scalar(name=name, tensor=tensor)

        self.merged_summary = tf.summary.merge_all()

        self.training_writer = tf.summary.FileWriter(logdir=os.path.join(self.get_metrics_path(), 'train'),
                                                     graph=tf.get_default_graph())
        self.validation_writer = tf.summary.FileWriter(logdir=os.path.join(self.get_metrics_path(), 'validation'),
                                                       graph=tf.get_default_graph())
        self.save_controller = SaveController(['recall'])

    def _get_input_feed(self, inputs, is_training):
        return self.model.get_input_feed(inputs, is_training)

    def feedforward_run(self, inputs, is_training):
        input_feed = self._get_input_feed(inputs, is_training)

        output_feed = [self.output_feed_total,
                       self.merged_summary]

        if is_training:
            output_feed.append(self.train_op)

        return self.sess.run(output_feed, input_feed)[:2]

    def save_weights(self, force=False):
        # save weights only if there are no validation scores yet, or metrics are improved on validation

        if self.step + 1 >= self._config.save_weights_start:
            if self.save_controller.are_scores_improved(self.validation_metrics) or force:
                checkpoint_file = os.path.join(self.get_checkpoint_path(), self._config.checkpoint_file)
                self.saver.save(self.sess, checkpoint_file, global_step=self.global_step + 1, write_meta_graph=False)
                logger.info('Weights dumped to %s', checkpoint_file)

    def restore_weights(self, path=None, mode='last'):
        result = super(Trainer, self).restore_weights(path, mode)

        if result:
            first_step = self.step

            self.training_writer.add_session_log(SessionLog(status=SessionLog.START), global_step=first_step)
            self.validation_writer.add_session_log(SessionLog(status=SessionLog.START), global_step=first_step)

            return True
        else:
            return False

    @staticmethod
    def _current_batch_size(inputs):
        return len(inputs['labels'])

    def train(self):
        metrics_sums = Counter()
        num_samples = 0
        training_start_time = time.time()

        for inputs in self._preprocessor.iterate_batches_train():
            batch_metrics, batch_summary = self.feedforward_run(inputs, is_training=True)

            step = self.step
            step_num = step + 1

            if hasattr(self, 'training_writer'):
                self.training_writer.add_summary(batch_summary, step)

            metrics_sums.update(Counter({name: batch_metric * self._current_batch_size(inputs)
                                         for name, batch_metric in batch_metrics.iteritems()}))
            num_samples += self._current_batch_size(inputs)

            if step_num % self._config.training_loss_freq == 0:
                output = 'training:   {}b'.format(step_num)
                for name in self.output_feed_total.keys():
                    output += '\t{}={:.8f}'.format(name, metrics_sums[name] / float(num_samples))
                output += '\t{:.3f}s'.format(time.time() - training_start_time)
                logger.info(output)

                metrics_sums = Counter()
                num_samples = 0
                training_start_time = time.time()

            if self._config.train_split < 1 and (step_num % self._config.validation_loss_freq == 0):
                self.validate(step)
                training_start_time = time.time()

            saved = False
            if self._config.save_weights_mode == 'last':
                if step_num == self._preprocessor.num_train_batches:
                    self.save_weights(force=True)
                    saved = True
            elif self._config.save_weights_mode == 'best':
                if step_num % self._config.checkpoint_freq_in_batches == 0:
                    self.save_weights()
                    saved = True
            else:
                raise ValueError('No such save_weigths_mode "{}"'.format(self._config.save_weights_mode))

            if saved:
                training_start_time = time.time()

            for callback in self._config.callbacks:
                callback.on_batch_end(self)

    def validate(self, step):
        validation_start_time = time.time()
        metrics_sums = Counter()
        num_samples = 0

        for inputs in self._preprocessor.iterate_batches_test():
            batch_metrics = self.feedforward_run(inputs, is_training=False)[0]
            metrics_sums.update(Counter({name: batch_metric * self._current_batch_size(inputs)
                                         for name, batch_metric in batch_metrics.iteritems()}))
            num_samples += self._current_batch_size(inputs)

        self.validation_metrics = Counter({name: metric_sum / float(num_samples)
                                           for name, metric_sum in metrics_sums.iteritems()})

        if hasattr(self, 'validation_writer'):
            self.validation_writer.add_summary(create_summary(self.validation_metrics), step)

        output = 'validation:   {}b'.format(step + 1)
        for name in self.output_feed_total.keys():
            output += '\t{}={:.8f}'.format(name, self.validation_metrics[name])
        timedelta = time.time() - validation_start_time
        output += '\t{:.3f}s'.format(timedelta)

        logger.info(output)

        return timedelta
