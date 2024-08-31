import tensorflow as tf
import argparse, codecs, readline
import numpy as np

from language_model import Language_model
from nn_utils import train, encode, get_checkpoint_path
from utils import load_dictionary, reverse_dictionary, Preprocessor
from utils import line_converter_for_lm
from utils import BOS, EOS, UNK



def run_lm(args, config, word_dct, trigram_dct):
    embedding_params = {'word_dct_size': len(word_dct),
                        'trigram_dct_size': len(trigram_dct),
                        'embedding_scope_name': 'reply_pool'}

    with tf.device('/gpu:'+str(args.gpu_id)):
        model = Language_model(name='reply',
                               learning_rate=args.learning_rate,
                               epsilon = args.epsilon,
                               grad_threshold=args.grad_threshold,
                               **embedding_params)

    if args.mode == 'train':
        train(args, config, model, word_dct, trigram_dct,
              line_converter=line_converter_for_lm)

    if args.mode == 'encode':
        encode(args, config, model, word_dct, trigram_dct,
               line_converter=line_converter_for_lm)

    if args.mode == 'talk':
        talk(args, config, model, word_dct, trigram_dct)



def talk(args, config, model, word_dct, trigram_dct, T=0.8):
    id_to_word = reverse_dictionary(word_dct)
    checkpoint_path = get_checkpoint_path(args.restore_path)
    print 'Restore', checkpoint_path

    with tf.Session(config=config) as sess:
        model.saver.restore(sess, checkpoint_path)
        while True:
            converter_params = {'word_dct': word_dct,
                                'trigram_dct': trigram_dct}
            columns_settings = '0'
            preprocessor = Preprocessor(columns_settings, args.strip_punctuation, args.keep,
                                        line_converter_for_lm, converter_params)
            line = codecs.decode(raw_input('> '), 'utf-8').rstrip('\n')
            while True:
                batch = np.array([preprocessor(line)], dtype=object)
                distr = model.get_next_word_distribution(sess, batch)[0]

                # no unk
                distr[-1] = 1e-10

                distr = np.exp(np.log(distr)/T)/np.sum(np.exp(np.log(distr)/T))
                next_word_id = np.random.choice(len(word_dct), size=1, p=distr)[0]
                next_word = id_to_word[next_word_id]
                if next_word == EOS or len(batch[0,0]) > 50:
                    print line
                    break
                line += (u' ' + next_word)



def main(args):
    word_dct = load_dictionary(args.word_dct_file, [BOS, EOS, UNK])
    trigram_dct = load_dictionary(args.trigram_dct_file)

    config = tf.ConfigProto()
    config.allow_soft_placement = True
    config.log_device_placement = False
    config.gpu_options.allow_growth = True

    run_lm(args, config, word_dct, trigram_dct)




if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--dataset', required=True)
    parser.add_argument('--validation-dataset')
    parser.add_argument('--word-dct-file', required=True)
    parser.add_argument('--trigram-dct-file', required=True)
    parser.add_argument('--columns', required=True)
    parser.add_argument('--yt', action='store_true')

    parser.add_argument('--batch-size', type=int, default=512)
    parser.add_argument('--max-num-epochs', type=int, default=100)

    parser.add_argument('--learning-rate', type=float, default=1e-3)
    parser.add_argument('--epsilon', type=float, default=1e-4)
    parser.add_argument('--grad-threshold', type=float, default=1.)

    parser.add_argument('--mode', default='train', choices=['train', 'encode', 'talk'])

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
