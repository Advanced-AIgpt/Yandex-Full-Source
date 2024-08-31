# -*- coding: utf-8 -*-

import tensorflow as tf
from itertools import izip
import numpy as np
import time, codecs, sys, os, readline
from nn_utils import get_checkpoint_path
from utils import iterate_batches, Preprocessor, parse_column_settings



def extract_lines(file_name, cols, line_indices=None):
    if line_indices is None:
        T = []
        with codecs.open(file_name, 'r', 'utf-8') as f:
            for line in f:
                parts = line.rstrip('\n').split('\t')
                T.append('\t'.join([parts[col] for col in cols]))
        return T

    dct = {}
    for idx, line_idx in enumerate(line_indices):
        if not line_idx in dct:
            dct[line_idx] = []
        dct[line_idx].append(idx)

    T = ['']*len(line_indices)
    with codecs.open(file_name, 'r', 'utf-8') as f:
        for line_idx, line in enumerate(f):
            if line_idx > max(line_indices):
                break
            if line_idx in dct:
                parts = line.rstrip('\n').split('\t')
                line = '\t'.join([parts[col] for col in cols])
                for idx in dct[line_idx]:
                    T[idx] = line
    return T


def iterate_repr_batches(sess, filename, columns, encoder_model, args, config,
                         word_dct, trigram_dct, wbigram_dct, line_converter):
    repr_batch = []
    for batch, _, _ in iterate_batches(filename, columns, args.batch_size,
                                       word_dct, trigram_dct, wbigram_dct,
                                       line_converter, args.strip_punctuation, args.keep):
        repr_batch.append(encoder_model.encode(sess, batch))
        if len(repr_batch) == args.repr_batch_size:
            yield np.vstack(repr_batch).astype(np.float32)
            repr_batch = []
    if repr_batch:
        yield np.vstack(repr_batch).astype(np.float32)


def save_nearest_replies(args, save_path, queries, top_scores, top_indices):
    dir_path = os.path.dirname(save_path)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)

    sorting_order = list(np.ix_(*[np.arange(i) for i in top_scores.shape]))
    sorting_order[0] = np.argsort(-top_scores, axis=0)
    top_scores = top_scores[sorting_order]
    top_indices = top_indices[sorting_order]

    context_reply_pairs = extract_lines(args.dataset, [int(col) for col in args.columns.split(',')], top_indices.flatten())
    context_reply_pairs = np.array(context_reply_pairs).reshape([len(top_indices), -1])

    with codecs.open(save_path, 'w', 'utf-8') as f:
        for query_idx in range(top_indices.shape[1]):
            for pool_idx in range(top_indices.shape[0]):
                context, reply = context_reply_pairs[pool_idx,query_idx].split('\t')
                f.write('%.3f\t%s\t%s\t%s\n' % (top_scores[pool_idx,query_idx], queries[query_idx], reply, context))


def reply_to_context_set(args, config, model, word_dct, trigram_dct,
                         wbigram_dct=None, line_converter=None):

    start_time = time.time()

    queries = extract_lines(args.queries, [int(col) for col in args.queries_columns.split(',')])
    queries_repr = []

    checkpoint_path = get_checkpoint_path(args.restore_path)
    print 'Restore', checkpoint_path

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, checkpoint_path)

        for batch_queries_repr in iterate_repr_batches(sess, args.queries, args.queries_columns, model.encoders[0], args, config,
                                                       word_dct, trigram_dct, wbigram_dct, line_converter):
            queries_repr.append(batch_queries_repr)

        queries_repr_T = np.vstack(queries_repr).T


        print 'queries representations are calculated'


        top_scores = np.zeros((2*args.n, len(queries)), dtype=np.float32)
        top_indices = np.zeros((2*args.n, len(queries)), dtype=np.int32)
        borderline = 0

        line_idx = 0

        pool_columns = args.columns.split(',')
        pool_contexts_columns = ''.join(pool_columns[:-1])
        pool_replies_columns = pool_columns[-1]

        for batch_pool_contexts_repr, batch_pool_replies_repr in izip(iterate_repr_batches(sess, args.dataset, pool_contexts_columns,
                                                    model.encoders[0], args, config, word_dct, trigram_dct, wbigram_dct, line_converter),
                                                                      iterate_repr_batches(sess, args.dataset, pool_replies_columns,
                                                    model.encoders[1], args, config, word_dct, trigram_dct, wbigram_dct, line_converter)):

            actual_batch_size = len(batch_pool_contexts_repr)

            batch_pool_repr = batch_pool_contexts_repr*(1-args.reply_coeff) + \
                              batch_pool_replies_repr*args.reply_coeff

            batch_scores = np.dot(batch_pool_repr, queries_repr_T)
            batch_indices = np.array([[line_idx+idx_in_batch]*len(queries)
                                       for idx_in_batch in xrange(actual_batch_size)])

            best_in_batch_order = list(np.ix_(*[np.arange(i) for i in batch_scores.shape]))

            if actual_batch_size > args.n:
                best_in_batch_order[0] = np.argpartition(-batch_scores, args.n, axis=0)[:args.n]


            best_in_batch_scores = batch_scores[best_in_batch_order]
            best_in_batch_indices = batch_indices[best_in_batch_order]

            update_size = len(best_in_batch_scores)

            top_scores[borderline:borderline+update_size] = best_in_batch_scores
            top_indices[borderline:borderline+update_size] = best_in_batch_indices

            borderline += update_size

            if borderline > args.n:
                partial_order = list(np.ix_(*[np.arange(i) for i in top_scores.shape]))
                partial_order[0] = np.argpartition(-top_scores, args.n, axis=0)
                top_scores = top_scores[partial_order]
                top_indices = top_indices[partial_order]
                borderline = args.n

            line_idx += actual_batch_size

            #if args.output_freq and line_idx % args.output_freq == 0:
            print >> sys.stderr, '%d lines processed, %ds elapsed \r' % (line_idx, time.time() - start_time),

            if args.checkpoint_freq > 0 and line_idx % args.checkpoint_freq == 0:
                save_nearest_replies(args, args.save_path+'-'+str(line_idx), queries,
                                     top_scores[:borderline], top_indices[:borderline])

    save_nearest_replies(args, args.save_path, queries, top_scores[:borderline], top_indices[:borderline])
    print '%d' % (time.time() - start_time)





def find_nearest_in_pool(sess, model, query, preprocessor, pool,
                         pool_contexts_repr_T, pool_replies_repr_T, reply_coeff, top_n):
    context_model = model.encoders[0]
    reply_model = model.encoders[1]
    query_preprocessed = np.array([preprocessor(query)], dtype=object)
    query_repr = context_model.encode(sess, query_preprocessed)[0]
    pool_repr_T = (1 - reply_coeff) * pool_contexts_repr_T + reply_coeff * pool_replies_repr_T
    scores = np.dot(query_repr, pool_repr_T)
    pairs_ids = np.argsort(-scores)[:top_n]
    return pool[pairs_ids], scores[pairs_ids]


def talk(args, config, model, word_dct, trigram_dct,
         wbigram_dct=None, line_converter=None):
    start_time = time.time()
    pool = np.array(extract_lines(args.dataset, [int(col) for col in args.columns.split(',')]))

    pool_columns = args.columns.split(',')
    pool_contexts_columns = ''.join(pool_columns[:-1])
    pool_replies_columns = pool_columns[-1]
    query_len = len(args.queries_columns.split(','))

    checkpoint_path = get_checkpoint_path(args.restore_path)
    print 'Restore', checkpoint_path

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, checkpoint_path)

        pool_contexts_repr = []
        pool_replies_repr = []
        for batch_pool_contexts_repr, batch_pool_replies_repr in izip(iterate_repr_batches(sess, args.dataset, pool_contexts_columns,
                                                    model.encoders[0], args, config, word_dct, trigram_dct, wbigram_dct, line_converter),
                                                                      iterate_repr_batches(sess, args.dataset, pool_replies_columns,
                                                    model.encoders[1], args, config, word_dct, trigram_dct, wbigram_dct, line_converter)):
            pool_contexts_repr.append(batch_pool_contexts_repr)
            pool_replies_repr.append(batch_pool_replies_repr)
            print >> sys.stderr, '%d batches processed\r' % len(pool_contexts_repr),

        pool_contexts_repr_T = np.vstack(pool_contexts_repr).T
        pool_replies_repr_T = np.vstack(pool_replies_repr).T

        print 'replies representations are calculated during %ds\n' % (time.time() - start_time),

        converter_params = {'word_dct': word_dct,
                            'trigram_dct': trigram_dct}
        if wbigram_dct:
            converter_params['wbigram_dct'] = wbigram_dct

        preprocessor = Preprocessor(args.queries_columns, args.strip_punctuation, args.keep,
                                    line_converter, converter_params)

        bot_turn = False
        query = [u'']*(query_len-1)
        if bot_turn:
            query.append(u'привет')
        else:
            query.append(u'')

        while True:
            if bot_turn:
                context_reply_pairs, scores = find_nearest_in_pool(sess, model, '\t'.join(query), preprocessor, pool,
                                                                   pool_contexts_repr_T, pool_replies_repr_T, args.reply_coeff, args.n)
                for idx, (context_reply_pair, score) in enumerate(zip(context_reply_pairs, scores)):
                    context, reply = context_reply_pair.split('\t')
                    if idx == 0:
                        print 'B:',
                    else:
                        print '  ',
                    print '[%.3f] %s | %s' % (score, reply, context)
            else:
                reply = codecs.decode(raw_input('H: '), 'utf-8').rstrip('\n')
                if reply == u'выкл':
                    break

            bot_turn = not bot_turn

            query.append(reply)
            query = query[-query_len:]
