# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import os
import argparse
import shutil
import tensorflow as tf

from keras import backend as K
from keras.utils.np_utils import convert_kernel
from keras.models import model_from_json

tf.python.control_flow_ops = tf


def convert(input_file, output_file=None):
    if not output_file:
        output_file = input_file
    shutil.copyfile(input_file + '.enc', output_file + '.enc')
    model = model_from_json(open(input_file).read())
    model.load_weights(input_file + '.h5')
    ops = []
    for layer in model.layers:
        if layer.__class__.__name__ in ['Convolution1D', 'Convolution2D', 'Convolution3D', 'AtrousConvolution2D']:
            original_w = K.get_value(layer.W)
            converted_w = convert_kernel(original_w)
            ops.append(tf.assign(layer.W, converted_w).op)
    K.get_session().run(ops)
    with open(output_file, 'w') as fout:
        fout.write(model.to_json())
    model.save_weights(output_file + '.h5', overwrite=os.path.exists(output_file + '.h5'))


def main():
    if K._backend != 'tensorflow':
        raise ValueError('Tensorflow backend should be set for weights conversion, '
                         'please check etting in ~/.keras/keras.json')
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', dest='input', required=True,
                        help='input JSON file with Keras model trained with Theano backend')
    parser.add_argument('-o', '--output', dest='output', required=False,
                        help='output JSON file with the same Keras model in Tensorflow format '
                             '(overwrites existing files if skipped)')
    args = parser.parse_args()
    if not args.output:
        args.output = args.input

    convert(args.input, args.output)


if __name__ == "__main__":
    main()
