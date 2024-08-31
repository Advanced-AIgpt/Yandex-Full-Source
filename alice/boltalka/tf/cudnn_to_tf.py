import tensorflow as tf

def cudnn_to_tf_lstm(sess, inner_scope, cudnn_model):
    cudnn_params = cudnn_model.get_canonical_weights()
    for layer in xrange(cudnn_model.num_layers):
        with tf.variable_scope(inner_scope):
            with tf.variable_scope('MultiRNNCell/Cell{}/LSTMCell'.format(layer)) as lstm_vs:
                params_dict = cudnn_params[layer]
                i = tf.concat(0, [
                    params_dict['input2input_gate_weights'],
                    params_dict['prev_recurrent2input_gate_weights']
                ])
                c = tf.concat(0, [
                    params_dict['input2cell_weights'],
                    params_dict['prev_recurrent2cell_weights']
                ])
                f = tf.concat(0, [
                    params_dict['input2forget_gate_weights'],
                    params_dict['prev_recurrent2forget_gate_weights']
                ])
                o = tf.concat(0, [
                    params_dict['input2output_gate_weights'],
                    params_dict['prev_recurrent2output_gate_weights']
                ])
                w = tf.concat(1, [i, c, f, o])
                W = tf.get_variable(name='W_0', initializer=w)

                b = tf.concat(0, [
                    params_dict['input_gate_biases'],
                    params_dict['cell_biases'],
                    params_dict['forget_gate_biases'],
                    params_dict['output_gate_biases']
                ])
                B = tf.get_variable(name='B', initializer=b)

                sess.run(W._initializer_op, { W._initializer_op.inputs[1]: w.eval() })
                sess.run(B._initializer_op, { B._initializer_op.inputs[1]: b.eval() })
