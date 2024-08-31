import tensorflow as tf
from tensorflow.contrib.tensorboard.plugins import projector
from tensorflow.core.util.event_pb2 import SessionLog
from tensorflow.python.framework.ops import IndexedSlices
import codecs, time, sys, os, math
import numpy as np
from collections import Counter
from copy import copy

from complex_encoder import Complex_encoder
from utils import iterate_batches

import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")


tf.set_random_seed(1)
np.random.seed(1)



def clip_by_norm_2(t, clip_norm):
    val = t.values if isinstance(t, IndexedSlices) else t
    clipped_val = tf.clip_by_norm(val, clip_norm)
    if isinstance(t, IndexedSlices):
        return IndexedSlices(clipped_val, t.indices, t.dense_shape)
    return clipped_val


def create_summary(dic):
    return tf.Summary(value=[tf.Summary.Value(tag=name, simple_value=value)
                             for name, value in dic.iteritems()])


def get_checkpoint_path(restore_path):
    if os.path.isdir(restore_path):
        ckpt = tf.train.get_checkpoint_state(restore_path)
        if ckpt and ckpt.model_checkpoint_path:
            checkpoint_path = ckpt.model_checkpoint_path
        else:
            print >> sys.stderr, 'Nothing to restore!'
            sys.exit(0)
    elif os.path.isfile(restore_path+'.meta'):
        checkpoint_path = restore_path
    else:
        print >> sys.stderr, 'Nothing to restore!'
        sys.exit(0)
    return checkpoint_path


def construct_encoder_config(encoder_model, embedding_params, name):
    context_len = len(embedding_params)
    if context_len == 1:
        return {'model': encoder_model,
                'name': name,
                'params': embedding_params[0]
                }
    subencoders_configs = [{'model': encoder_model, 'name': str(idx), 'params': embedding_params[idx]}
                           for idx in xrange(context_len)]
    return {'model': Complex_encoder,
            'name': name,
            'params': {'encoders_configs': subencoders_configs}
            }


def transfer_weights(args, config, mapping, keep_other=False):
    checkpoint_path = get_checkpoint_path(args.restore_path)
    saver_dssm = tf.train.import_meta_graph(checkpoint_path+'.meta')

    new_variables = {}
    for v in tf.global_variables():
        if v.op.name in mapping:
            new_variables[mapping[v.op.name]] = v
        elif keep_other:
            new_variables[v.op.name] = v

    saver_new = tf.train.Saver(new_variables, max_to_keep=1)

    with tf.Session(config=config) as sess:
        saver_dssm.restore(sess, checkpoint_path)
        if not os.path.exists(args.save_path):
            os.makedirs(args.save_path)
        save_path = os.path.join(args.save_path, args.checkpoint_file)
        saver_new.save(sess, save_path)
    tf.reset_default_graph()
    return save_path


class Save_controller(object):
    def __init__(self, metrics_names):
        assert metrics_names[0] == 'loss'
        if len(metrics_names) == 2:
            assert metrics_names[1] == 'recall' or metrics_names[1] == 'accuracy'
        self.best_saved_scores = None
        self.metrics_names = metrics_names

    def are_scores_improved(self, new_scores):
        if self.best_saved_scores is None:
            self.best_saved_scores = copy(new_scores)
            return True
        if new_scores[self.metrics_names[0]] >= self.best_saved_scores[self.metrics_names[0]] and \
                            len(self.metrics_names) != 2 or new_scores[self.metrics_names[1]] <= self.best_saved_scores[self.metrics_names[1]]:
            return False
        self.best_saved_scores[self.metrics_names[0]] = min(self.best_saved_scores[self.metrics_names[0]], new_scores[self.metrics_names[0]])
        self.best_saved_scores[self.metrics_names[1]] = max(self.best_saved_scores[self.metrics_names[1]], new_scores[self.metrics_names[1]])
        return True


def validate(args, sess, model, word_dct, trigram_dct,
             wbigram_dct=None, line_converter=None):

    metrics_sums = Counter()
    num_samples = 0
    msg = ''
    for batch, _, _ in iterate_batches(args.validation_dataset, args.columns, args.batch_size,
                                       word_dct, trigram_dct, wbigram_dct,
                                       line_converter, args.strip_punctuation, args.keep,
                                       args.yt, args.skip_to_count):
        batch_metrics = model(sess, batch, is_training=False)[0]
        metrics_sums += Counter({name: batch_metric * len(batch)
                                 for name, batch_metric in batch_metrics.iteritems()})
        num_samples += len(batch)
        msg = 'validation: {} samples processed'.format(num_samples)
        print >> sys.stderr, msg + '\r',
    print >> sys.stderr, ' ' * len(msg) + '\r',
    metrics = Counter({name: metric_sum / num_samples
                       for name, metric_sum in metrics_sums.iteritems()})
    return metrics, create_summary(metrics)


def train(args, config, model, word_dct, trigram_dct, wbigram_dct=None,
          line_converter=None, restore_path=None, saver=None):

    if restore_path is None:
        restore_path = args.restore_path
    if saver is None:
        saver = model.saver

    if not os.path.exists(args.save_path):
        os.makedirs(args.save_path)

    # loss visualization
    training_writer = tf.summary.FileWriter(os.path.join(args.save_path, 'train'))
    validation_writer = tf.summary.FileWriter(os.path.join(args.save_path, 'validation'))

    """
    # embeddings visualization
    embeddings_writer = tf.summary.FileWriter(os.path.join(args.save_path, 'projector'))
    embeddings_config = projector.ProjectorConfig()
    embedding = embeddings_config.embeddings.add()
    embedding.metadata_path = args.word_dct_file
    projector.visualize_embeddings(embeddings_writer, embeddings_config)
    """

    first_epoch_idx = 0
    first_batch_idx = 0

    if args.yt:
        num_samples_in_dataset = yt.row_count(args.dataset)
    else:
        with open(args.dataset) as f:
            num_samples_in_dataset = sum(1 for _ in f)

    num_batches_in_epoch = int(math.ceil(num_samples_in_dataset / float(args.batch_size)))

    if args.checkpoint_freq > 0:
        checkpoint_freq_in_batches = args.checkpoint_freq
    else:
        checkpoint_freq_in_batches = -args.checkpoint_freq*num_batches_in_epoch


    with tf.Session(config=config) as sess:
        sess.run(tf.global_variables_initializer())
        backward_shift = 0

        if restore_path:
            checkpoint_path = get_checkpoint_path(restore_path)
            print 'Restore', checkpoint_path
            saver.restore(sess, checkpoint_path)
            first_step = model.global_step.eval()
            backward_shift = first_step

            training_writer.add_session_log(SessionLog(status=SessionLog.START),
                                            global_step=first_step)
            validation_writer.add_session_log(SessionLog(status=SessionLog.START),
                                              global_step=first_step)

            if args.continue_training:
                assert first_step == int(checkpoint_path.split('-')[-1])
                first_batch_idx = first_step % num_batches_in_epoch
                first_epoch_idx = first_step // num_batches_in_epoch
                backward_shift = 0

            print 'Train from {}e {}b'.format(first_epoch_idx, first_batch_idx)

        metrics_names = model.get_metrics_names()
        validation_metrics = None
        save_controller = Save_controller(metrics_names)

        metrics_sums = Counter()
        num_samples = 0

        training_start_time = time.time()

        for batch, batch_idx, epoch_idx in iterate_batches(args.dataset, args.columns, args.batch_size,
                                                           word_dct, trigram_dct, wbigram_dct,
                                                           line_converter, args.strip_punctuation, args.keep,
                                                           args.yt, args.skip_to_count,
                                                           args.word_drop_prob,
                                                           first_epoch_idx, first_batch_idx,
                                                           infinite=True):

            if epoch_idx - first_epoch_idx == args.max_num_epochs:
                break

            step = model.global_step.eval() - backward_shift

            batch_metrics, batch_summary = model(sess, batch)[:2]
            training_writer.add_summary(batch_summary, step)

            metrics_sums += Counter({name: batch_metric * len(batch)
                                     for name, batch_metric in batch_metrics.iteritems()})
            num_samples += len(batch)

            if (batch_idx + 1) % args.training_loss_freq == 0:
                print 'training:   {}e {}b'.format(epoch_idx, batch_idx+1),
                for name in metrics_names:
                    print '\t{}={:.8f}'.format(name, metrics_sums[name] / num_samples),
                print '\t{:.3f}s'.format(time.time() - training_start_time)

                metrics_sums = Counter()
                num_samples = 0
                training_start_time = time.time()

            if args.validation_dataset and (((batch_idx + 1) % args.validation_loss_freq == 0) or \
                                            (len(batch) != args.batch_size)):
                validation_start_time = time.time()
                validation_metrics, validation_summary = validate(args, sess, model, word_dct,
                                                                  trigram_dct, wbigram_dct,
                                                                  line_converter)
                validation_writer.add_summary(validation_summary, step)

                print 'validation: {}e {}b'.format(epoch_idx, batch_idx+1),
                for name in metrics_names:
                    print '\t{}={:.8f}'.format(name, validation_metrics[name]),
                print '\t{:.3f}s'.format(time.time() - validation_start_time)

            if (step + 1) % checkpoint_freq_in_batches == 0:
                # save weights only if there are no validation scores yet, or metrics are improved on validation
                if save_controller.are_scores_improved(validation_metrics):
                    model.saver.save(sess, os.path.join(args.save_path, args.checkpoint_file),
                                     global_step=model.global_step)


def encode(args, config, model, word_dct, trigram_dct,
           wbigram_dct=None, line_converter=None):
    save_dir_path = os.path.dirname(args.save_path)

    if save_dir_path != '' and not os.path.exists(save_dir_path):
        os.makedirs(save_dir_path)

    checkpoint_path = get_checkpoint_path(args.restore_path)
    print 'Restore', checkpoint_path

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, checkpoint_path)
        with codecs.open(args.save_path, 'w', 'utf-8') as f:
            for batch, batch_idx, _ in iterate_batches(args.dataset, args.columns, args.batch_size,
                                                       word_dct, trigram_dct, wbigram_dct,
                                                       line_converter, args.strip_punctuation, args.keep,
                                                       args.yt):
                np.savetxt(f, model.encode(sess, batch), delimiter='\t')
                print >> sys.stderr, args.batch_size*(batch_idx+1), 'lines\r',
