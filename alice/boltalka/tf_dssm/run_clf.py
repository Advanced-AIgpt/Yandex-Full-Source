import tensorflow as tf
import argparse, codecs, sys, os, readline
import numpy as np

from recurrent_encoder import Recurrent_encoder
from bow_encoder import BOW_encoder
from bow_with_wbigrams_encoder import BOW_with_wbigrams_encoder
from complex_encoder import Complex_encoder
from classifier import Classifier
from nn_utils import construct_encoder_config, transfer_weights, train, encode, get_checkpoint_path
from utils import iterate_batches, load_dictionary, Preprocessor
from utils import line_converter_for_lm, line_converter_for_bow, line_converter_for_bow_dense
from utils import BOS, EOS, UNK



def run_clf(args, config, word_dct, trigram_dct, wbigram_dct):
    embedding_params = {'word_dct_size': len(word_dct),
                        'trigram_dct_size': len(trigram_dct),
                        'embedding_scope_name': 'reply_pool'}

    if wbigram_dct:
        embedding_params['wbigram_dct_size'] = len(wbigram_dct)

    ENCODER = BOW_with_wbigrams_encoder

    if ENCODER == Recurrent_encoder:
        line_converter = line_converter_for_lm
    else:
        line_converter = line_converter_for_bow

    encoder_config = construct_encoder_config(ENCODER, [embedding_params], 'reply')


    if args.transfer_learning:
        mapping = {'reply_pool/word_embeddings_matrix': embedding_params['embedding_scope_name']+'/word_embeddings_matrix',
                   'reply_pool/trigram_embeddings_matrix': embedding_params['embedding_scope_name']+'/trigram_embeddings_matrix'}
        restore_path = transfer_weights(args, config, mapping)


    with tf.device('/gpu:'+str(args.gpu_id)):
        model = Classifier(encoder_config,
                           learning_rate=args.learning_rate,
                           epsilon = args.epsilon,
                           grad_threshold=args.grad_threshold)


    if args.transfer_learning:
        saver = tf.train.Saver([tf.get_default_graph().get_tensor_by_name(name+':0') for name in mapping.values()], max_to_keep=1)


    if args.mode == 'train':
        if not args.transfer_learning:
            restore_path = None
            saver = None
        train(args, config, model, word_dct, trigram_dct, wbigram_dct,
              line_converter, restore_path, saver)
    if args.mode == 'encode':
        encode(args, config, model.encoder, word_dct, trigram_dct, wbigram_dct,
               line_converter)
    if args.mode == 'predict':
        predict(args, config, model, word_dct, trigram_dct, wbigram_dct,
                line_converter)



def predict(args, config, model, word_dct, trigram_dct,
            wbigram_dct=None, line_converter=None, online=False):
    save_dir_path = os.path.dirname(args.save_path)
    if save_dir_path != '' and not os.path.exists(save_dir_path):
        os.makedirs(save_dir_path)
    checkpoint_path = get_checkpoint_path(args.restore_path)
    print 'Restore', checkpoint_path

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, checkpoint_path)
        if online:
            converter_params = {'word_dct': word_dct,
                                'trigram_dct': trigram_dct}
            if wbigram_dct:
                converter_params['wbigram_dct'] = wbigram_dct
            preprocessor = Preprocessor(args.columns, args.strip_punctuation, args.keep,
                                        line_converter, converter_params)
            while True:
                line = codecs.decode(raw_input('> '), 'utf-8').rstrip('\n')
                batch = np.array([preprocessor(line)], dtype=object)
                print '{:.2f}'.format(model.predict(sess, batch)[0])
        else:
            with codecs.open(args.save_path, 'w', 'utf-8') as f:
                for batch, batch_idx, _ in iterate_batches(args.dataset, args.columns, args.batch_size,
                                                           word_dct, trigram_dct, wbigram_dct,
                                                           line_converter, args.strip_punctuation, args.keep,
                                                           args.yt):
                    np.savetxt(f, model.predict(sess, batch), delimiter='\t', fmt='%.10f')
                    print >> sys.stderr, args.batch_size*(batch_idx+1), 'lines\r',




def main(args):
    word_dct = load_dictionary(args.word_dct_file, [BOS, EOS, UNK])
    trigram_dct = load_dictionary(args.trigram_dct_file)
    wbigram_dct = None if args.wbigram_dct_file is None else load_dictionary(args.wbigram_dct_file)

    config = tf.ConfigProto()
    config.allow_soft_placement = True
    config.log_device_placement = False
    config.gpu_options.allow_growth = True

    run_clf(args, config, word_dct, trigram_dct, wbigram_dct)




if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--dataset', required=True)
    parser.add_argument('--validation-dataset')
    parser.add_argument('--word-dct-file', required=True)
    parser.add_argument('--trigram-dct-file', required=True)
    parser.add_argument('--wbigram-dct-file', default=None)
    parser.add_argument('--columns', required=True)
    parser.add_argument('--yt', action='store_true')
    parser.add_argument('--skip-to-count', type=int, default=0)

    parser.add_argument('--batch-size', type=int, default=512)
    parser.add_argument('--max-num-epochs', type=int, default=100)

    parser.add_argument('--learning-rate', type=float, default=1e-3)
    parser.add_argument('--epsilon', type=float, default=1e-4)
    parser.add_argument('--grad-threshold', type=float, default=1.)

    parser.add_argument('--mode', default='train', choices=['train', 'encode', 'predict'])

    parser.add_argument('--save-path', default='save')
    parser.add_argument('--checkpoint-file', default='model.ckpt')
    parser.add_argument('--checkpoint-freq', type=int, default=-1, help='x batches if x > 0 else (-x) epochs')
    parser.add_argument('--restore-path')
    parser.add_argument('--transfer-learning', action='store_true')
    parser.add_argument('--continue-training', action='store_true')

    parser.add_argument('--training-loss-freq', type=int, default=50)
    parser.add_argument('--validation-loss-freq', type=int, default=10000)

    parser.add_argument('--gpu-id', type=int, default=0)

    parser.add_argument('--word-drop-prob', type=float, default=0.0)
    parser.add_argument('--strip-punctuation', action='store_true')
    parser.add_argument('--keep', type=str, default='', help='actual only if --strip-punctuation')

    args = parser.parse_args()

    main(args)
