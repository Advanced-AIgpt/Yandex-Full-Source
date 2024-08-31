import tensorflow as tf
import argparse

from recurrent_encoder import Recurrent_encoder
from bow_encoder import BOW_encoder
from bow_with_wbigrams_encoder import BOW_with_wbigrams_encoder
from complex_encoder import Complex_encoder
from dssm import DSSM
from nn_utils import construct_encoder_config, train, encode
from utils import load_dictionary
from utils import line_converter_for_lm, line_converter_for_bow, line_converter_for_bow_dense
from utils import BOS, EOS, UNK
from dssm_replier import talk, reply_to_context_set

from dssm_metrics import make_loss_semihard, make_loss_semihard_and_manual


def run_dssm(args, config, word_dct, trigram_dct, wbigram_dct):
    embedding_params_context = {'word_dct_size': len(word_dct),
                                'trigram_dct_size': len(trigram_dct),
                                'embedding_scope_name': 'context_pool'}

    embedding_params_reply = {'word_dct_size': len(word_dct),
                              'trigram_dct_size': len(trigram_dct),
                              'embedding_scope_name': 'reply_pool'}

    if wbigram_dct:
        embedding_params_context['wbigram_dct_size'] = len(wbigram_dct)
        embedding_params_reply['wbigram_dct_size'] = len(wbigram_dct)



    ENCODER = BOW_with_wbigrams_encoder

    if ENCODER == Recurrent_encoder:
        line_converter = line_converter_for_lm
    else:
        line_converter = line_converter_for_bow

    left_encoder_config = construct_encoder_config(ENCODER, [embedding_params_context], 'context')
    right_encoder_config = construct_encoder_config(ENCODER, [embedding_params_reply], 'reply')

    loss_maker = make_loss_semihard

    assert (args.skip_to_count > 0) == (args.columns[-1] == ']') == (loss_maker == make_loss_semihard_and_manual)


    with tf.device('/gpu:'+str(args.gpu_id)):
        model = DSSM([left_encoder_config, right_encoder_config],
                     loss_maker=loss_maker,
                     learning_rate=args.learning_rate,
                     epsilon = args.epsilon,
                     grad_threshold=args.grad_threshold,
                     num_negatives=args.num_negatives,
                     triplet_loss_threshold=args.triplet_loss_threshold,
                     left_anchor_prob=args.left_anchor_prob,
                     are_there_labels=args.columns[-1] == ']')

    if args.mode == 'train':
        train(args, config, model, word_dct, trigram_dct,
              wbigram_dct, line_converter)

    if args.mode == 'encode':
        encode(args, config, model.encoders[args.encoder_idx], word_dct,
               trigram_dct, wbigram_dct, line_converter)

    if args.mode == 'reply':
        if args.queries:
            reply_to_context_set(args, config, model, word_dct, trigram_dct,
                                 wbigram_dct, line_converter)
        else:
            talk(args, config, model, word_dct, trigram_dct,
                 wbigram_dct, line_converter)



def main(args):
    word_dct = load_dictionary(args.word_dct_file, [BOS, EOS, UNK])
    trigram_dct = load_dictionary(args.trigram_dct_file)
    wbigram_dct = None if args.wbigram_dct_file is None else load_dictionary(args.wbigram_dct_file)

    config = tf.ConfigProto()
    config.allow_soft_placement = True
    config.log_device_placement = False
    config.gpu_options.allow_growth = True

    run_dssm(args, config, word_dct, trigram_dct, wbigram_dct)




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

    parser.add_argument('--num-negatives', type=int, default=1)
    parser.add_argument('--triplet-loss-threshold', type=float, default=0.1)
    parser.add_argument('--left-anchor-prob', type=float, default=0.5)

    parser.add_argument('--mode', default='train', choices=['train', 'encode', 'reply'])
    # encode
    parser.add_argument('--encoder-idx', type=int, default=0, help='actual only in encode mode')
    # reply
    parser.add_argument('--queries', help='actual only in reply mode')
    parser.add_argument('--queries-columns', help='actual only in reply mode')
    parser.add_argument('--reply-coeff', type=float, default=1.0, help='actual only in reply mode')
    parser.add_argument('--repr-batch-size', type=int, default=100, help='actual only in reply mode')
    parser.add_argument('-n', type=int, default=10, help='actual only in reply mode')

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
