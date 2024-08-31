# coding=utf-8
import os
import sys
import argparse
import tensorflow as tf
import time
import codecs
import numpy as np

import data_utils
from data_utils import *
from cudnn_seq2seq import CudnnSeq2SeqModel
from cudnn_multi_gpu_seq2seq import CudnnMultiGpuSeq2SeqModel
from semi_cudnn_seq2seq import SemiCudnnSeq2SeqModel

sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
sys.stdin = os.fdopen(sys.stdin.fileno(), 'r', 0)
#sys.stdin = codecs.getreader('utf-8')(sys.stdin)

def compute_validation_loss(args, sess, model, dct, bpe_merges):
    loss = 0.0
    num_samples = 0
    msg = ''
    for batch in iterate_batches(args.validation_dataset, dct, args.batch_size, bpe_merges=bpe_merges):
        loss += model(sess, batch, is_training=False)[0] * len(batch)
        num_samples += len(batch)
        msg = 'validation: {} samples processed, loss {}'.format(num_samples, loss / num_samples)
        print >> sys.stderr, msg + '\r',
    print >> sys.stderr, ' ' * len(msg) + '\r',
    return loss / num_samples

def train(args, config, model, dct, bpe_merges):
    with tf.Session(config=config) as sess:
        if not args.restore_path:
            sess.run(tf.initialize_all_variables())
        else:
            model.saver.restore(sess, args.restore_path)

        writer = tf.train.SummaryWriter('.', sess.graph)

        for batch in iterate_batches(args.train_dataset, dct, args.batch_size, infinite=True, bpe_merges=bpe_merges, num_skip_batches=args.num_skip_batches):
            start_time = time.time()
            loss = model(sess, batch)[0]
            step = model.global_step.eval()
            print 'train:\t{}\t{}\t{}'.format(step, loss, time.time() - start_time)

            if step % args.steps_per_checkpoint == 0:
                checkpoint_path = './model.bckp'
                model.saver.save(sess, checkpoint_path, global_step=model.global_step)

            if step % args.steps_per_validation == 0:
                if args.validation_dataset:
                    start_time = time.time()
                    validation_loss = compute_validation_loss(args, sess, model, dct, bpe_merges)
                    print 'validation:\t{}\t{}\t{}'.format(step, validation_loss, time.time() - start_time)

def get_restore_path(args):
    restore_path = args.restore_path
    if not restore_path:
        checkpoint = tf.train.get_checkpoint_state('.')
        restore_path = checkpoint.model_checkpoint_path
    return restore_path

def talk(args, config, model, dct, tokens, bpe_merges):
    if type(args.gpu_id) is list:
        args.gpu_id = args.gpu_id[0]

    restore_path = get_restore_path(args)

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, restore_path)
        SemiCudnnSeq2SeqModel.load_from_cudnn_seq2seq(sess, model)
        vs = tf.get_variable_scope()
        vs.reuse_variables()

        with tf.device('/gpu:{}'.format(args.gpu_id)):
            decoder_model = SemiCudnnSeq2SeqModel(dct=dct,
                                                  embedding_size=args.embedding_size,
                                                  lstm_size=args.lstm_size,
                                                  num_layers=args.num_layers,
                                                  max_gradient_norm=args.max_gradient_norm,
                                                  max_input_sequence_length=args.max_context_length,
                                                  max_output_sequence_length=args.max_reply_length,
                                                  softmax_num_samples=args.softmax_num_samples,
                                                  #optimizer=tf.train.AdamOptimizer(args.learning_rate, epsilon=1e-2),
                                                  decode=True,
                                                  temperature=args.temperature)

        """
        validation_loss = compute_validation_loss(args, sess, decoder_model, dct, bpe_merges)
        print validation_loss
        exit(0)
        #"""

        context = []
        while True:
            print '>',
            line = codecs.decode(raw_input().strip(), 'utf-8')

            turn = line.strip()
            if args.bpe_dict:
                turn = apply_bpe_to_dataset_line(turn, bpe_merges)
            if turn: # talking with yourself
                context.append(turn)

            while len(context) > args.max_context_turns:
                context.pop(0)

            batch = get_context_for_decoder(context, dct)
            _, reply_ids = decoder_model(sess, batch, is_training=False)

            reply = ids_to_text(reply_ids, tokens)
            if reply.endswith(data_utils._EOS):
                reply = reply[:-len(data_utils._EOS)].rstrip()

            if args.bpe_dict:
                reply = ''.join(reply.split(' ')).replace(args.bpe_sentinel, ' ')
            print reply

            context.append(reply)

def predict(args, config, model, dct, bpe_merges):
    restore_path = get_restore_path(args)

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, restore_path)

        for batch in iterate_batches(args.validation_dataset, dct, args.batch_size, bpe_merges=bpe_merges):
            start_time = time.time()
            probas = model.predict_proba(sess, batch)
            for sample_probas in probas:
                for p in sample_probas:
                    print p,
                print

def evaluate(args, config, model, dct, bpe_merges):
    restore_path = get_restore_path(args)

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, restore_path)
        validation_loss = compute_validation_loss(args, sess, model, dct, bpe_merges)
        print validation_loss

def main(args):
    if args.bpe_dict:
        dct, tokens, bpe_merges = load_bpe_dictionary(args.dict_file)
    else:
        dct, tokens = load_dictionary(args.dict_file)
        bpe_merges = None

    if len(args.gpu_id) == 1:
        args.gpu_id = args.gpu_id[0]
        with tf.device('/gpu:{}'.format(args.gpu_id)):
            model = CudnnSeq2SeqModel(dct=dct,
                                      embedding_size=args.embedding_size,
                                      lstm_size=args.lstm_size,
                                      num_layers=args.num_layers,
                                      max_gradient_norm=args.max_gradient_norm,
                                      max_input_sequence_length=args.max_context_length,
                                      max_output_sequence_length=args.max_reply_length,
                                      softmax_num_samples=args.softmax_num_samples,
                                      optimizer=tf.train.AdamOptimizer(args.learning_rate, epsilon=args.epsilon))
    else:
        model = CudnnMultiGpuSeq2SeqModel(dct=dct,
                                          embedding_size=args.embedding_size,
                                          lstm_size=args.lstm_size,
                                          num_layers=args.num_layers,
                                          max_gradient_norm=args.max_gradient_norm,
                                          max_input_sequence_length=args.max_context_length,
                                          max_output_sequence_length=args.max_reply_length,
                                          softmax_num_samples=args.softmax_num_samples,
                                          optimizer=tf.train.AdamOptimizer(args.learning_rate, epsilon=args.epsilon),
                                          gpus=args.gpu_id)

    config = tf.ConfigProto()
    config.allow_soft_placement = True
    config.log_device_placement = False
    config.gpu_options.allow_growth = True

    if args.mode == 'train':
        train(args, config, model, dct, bpe_merges)

    if args.mode == 'talk':
        talk(args, config, model, dct, tokens, bpe_merges)

    if args.mode == 'predict':
        predict(args, config, model, dct, bpe_merges)

    if args.mode == 'evaluate':
        evaluate(args, config, model, dct, bpe_merges)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--train-dataset', default='')
    parser.add_argument('--validation-dataset', default='')
    parser.add_argument('--dict-file', required=True)
    parser.add_argument('--bpe-dict', action='store_true', default=False)
    parser.add_argument('--bpe-sentinel', default='-')
    parser.add_argument('--learning-rate', type=float, default=0.005)
    parser.add_argument('--epsilon', type=float, default=1e-2)
    parser.add_argument('--max-gradient-norm', type=float, default=1.0)
    parser.add_argument('--max-context-length', type=int, default=300)
    parser.add_argument('--max-reply-length', type=int, default=32)
    parser.add_argument('--batch-size', type=int, default=32)
    parser.add_argument('--embedding-size', type=int, default=512)
    parser.add_argument('--lstm-size', type=int, default=1024)
    parser.add_argument('--num-layers', type=int, default=1)
    parser.add_argument('--softmax-num-samples', type=int, default=0)
    parser.add_argument('--temperature', type=float, default=1.0)
    parser.add_argument('--steps-per-checkpoint', type=int, default=5000)
    parser.add_argument('--steps-per-validation', type=int, default=10000)
    parser.add_argument('--gpu-id', type=lambda x: map(int, x.split(',')), default='0')
    parser.add_argument('--mode', default='train', help='One of { train, talk, predict, evaluate }')
    parser.add_argument('--max-context-turns', type=int, default=1, help='Length of context for talk-mode.')
    parser.add_argument('--restore-path', default='', help='Path to model.bckp-XXX. Defaults to autodetecting.')
    parser.add_argument('--num-skip-batches', type=int, default=0, help='Number of first batches to skip.')
    args = parser.parse_args()

    main(args)
