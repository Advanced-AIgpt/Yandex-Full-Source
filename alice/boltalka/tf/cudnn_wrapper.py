import tensorflow as tf

def _have_gpu():
    me = _have_gpu
    if not hasattr(me, 'answer'):
        from tensorflow.python.client import device_lib # undocumented
        devices = device_lib.list_local_devices()
        gpus = [d for d in devices if d.device_type == 'GPU']
        me.answer = True if gpus else False
    return me.answer

class CudnnLSTM_wrapper(object):
    def __init__(self, num_layers, num_units, input_size, input_mode):
        self.num_layers = num_layers
        self.num_units = num_units
        #self.input_size = input_size
        #self.input_mode = input_mode

    def __call__(self, input_data, input_h, input_c, params, is_training):
        num_layers = self.num_layers
        num_units = self.num_units
        input_data_shape = tf.shape(input_data)
        batch_size = input_data_shape[1]
        input_size = input_data_shape[2]
        inputs = tf.unpack(input_data)

        scope=tf.get_variable_scope()
        cell = tf.nn.rnn_cell.LSTMCell(num_units=num_units, state_is_tuple=True, forget_bias=0.0)
        cell = tf.nn.rnn_cell.MultiRNNCell([cell] * num_layers)
        if False:
            if input_h is None: input_h = tf.zeros([num_layers, batch_size, num_units])
            if input_c is None: input_c = tf.zeros([num_layers, batch_size, num_units])
            initial_state=[(input_c, input_h)] * num_layers
        else:
            initial_state=cell.zero_state(batch_size, tf.float32)
        output, _state = tf.nn.rnn(cell, inputs, initial_state=initial_state, dtype=tf.float32, scope=scope)
        output = tf.stack(output)
        _h = [t.h for t in _state]
        _c = [t.c for t in _state]
        return output, _h, _c # or return output, None, None

class CanonicalCudnnLstm(object):
    INPUT_TYPES = ['input', 'prev_recurrent']
    PARAMS_TYPES = ['input_gate', 'forget_gate', 'cell', 'output_gate']

    def __init__(self,
                 num_layers,
                 num_units,
                 input_size,
                 weights_initializer,
                 biases_initializer,
                 forget_bias=0.0):

        self.num_layers = num_layers
        self.num_units = num_units
        self.input_size = input_size

        weights = []
        for layer in xrange(num_layers):
            for input_type in type(self).INPUT_TYPES:
                for params_type in type(self).PARAMS_TYPES:
                    num_inputs = input_size if input_type == 'input' and layer == 0 else num_units
                    w = self._create_weights(num_units, num_inputs, weights_initializer)
                    weights.append(tf.reshape(w, [-1]))

        biases = []
        for layer in xrange(num_layers):
            for params_type in type(self).PARAMS_TYPES:
                b = self._create_biases(num_units, biases_initializer)
                if params_type == 'forget_gate' and forget_bias != 0.0:
                    b = b + forget_bias * tf.ones([num_units])
                biases.append(b)
            # dummy biases
            for params_type in type(self).PARAMS_TYPES:
                dummy = tf.zeros([num_units])
                biases.append(dummy)

        initializer = tf.concat(0, weights + biases)
        def _initializer(shape, dtype=tf.float32, partition_info=None):
            return initializer
        self.params = tf.get_variable(shape=initializer.get_shape(), initializer=_initializer, name='cudnn_lstm_params')
        if _have_gpu():
            self.model = tf.contrib.cudnn_rnn.CudnnLSTM(num_layers, num_units, input_size, "linear_input")
        else:
            self.model = CudnnLSTM_wrapper(num_layers, num_units, input_size, "linear_input")
            self.scope = tf.get_variable_scope()

    def set_tf_vars(self, sess):
        from cudnn_to_tf import cudnn_to_tf_lstm
        with tf.variable_scope(self.scope, reuse=True) as scope:
            cudnn_to_tf_lstm(sess, scope, self)

    def get_canonical_weights(self):
        result = []
        offset = 0
        for layer in xrange(self.num_layers):
            params_dict = {}
            for input_type in type(self).INPUT_TYPES:
                for params_type in type(self).PARAMS_TYPES:
                    num_inputs = self.input_size if input_type == 'input' and layer == 0 else self.num_units
                    layer_size = num_inputs * self.num_units

                    matrix_name = input_type + '2' + params_type + '_weights'
                    params_dict[matrix_name] = tf.transpose(tf.reshape(self.params[offset:offset + layer_size], [self.num_units, num_inputs]))
                    offset += layer_size

            result.append(params_dict)

        for layer in xrange(self.num_layers):
            for input_type in type(self).INPUT_TYPES:
                for params_type in type(self).PARAMS_TYPES:
                    biases_name = params_type + '_biases'
                    if biases_name not in result[layer]:
                        result[layer][biases_name] = self.params[offset:offset + self.num_units]
                    else:
                        result[layer][biases_name] = result[layer][biases_name] + self.params[offset:offset + self.num_units]
                    offset += self.num_units

        return result

    def _create_weights(self, num_units, input_size, initializer):
        return initializer(shape=[num_units, input_size], dtype=tf.float32)

    def _create_biases(self, num_units, initializer):
        return initializer(shape=[num_units], dtype=tf.float32)

    def __call__(self, input_data, input_h=None, input_c=None, sequences_lengths=None, is_training=True):
        batch_size = tf.shape(input_data)[1]
        if input_h is None:
            input_h = tf.zeros([self.num_layers, batch_size, self.num_units])
        if input_c is None:
            input_c = tf.zeros([self.num_layers, batch_size, self.num_units])

        output, _h, _c = self.model(input_data, input_h, input_c, self.params, is_training)
        if sequences_lengths is None:
            return output

        max_sequence_length = tf.shape(input_data)[0]
        last_token_ids = sequences_lengths - 1
        last_tokens_one_hot = tf.one_hot(last_token_ids, max_sequence_length)
        last_tokens_one_hot = tf.expand_dims(tf.transpose(last_tokens_one_hot), -1)
        last_tokens_mask = tf.tile(last_tokens_one_hot, [1, 1, self.num_units])

        output = tf.reduce_sum(output * last_tokens_mask, 0)
        return output

def _testCudnn(input_data, num_layers, num_units, input_size, batch_size, seed, scope=None):
    #w = tf.uniform_unit_scaling_initializer(seed=seed)
    w = tf.random_uniform_initializer(-0.1, 0.1, seed=seed)
    #w = tf.constant_initializer(1.0)
    b = tf.constant_initializer(0.0)
    model = CanonicalCudnnLstm(num_layers, num_units, input_size, w, b, forget_bias=0.0)

    input_h = tf.zeros([num_layers, batch_size, num_units])
    input_c = tf.zeros([num_layers, batch_size, num_units])
    seq_length = None
    #seq_length = tf.random_uniform([batch_size], 1, input_data.get_shape()[0] + 1, dtype=tf.int32, seed=seed)

    output = model(
        input_data=input_data,
        input_h=input_h,
        input_c=input_c,
        sequences_lengths=seq_length,
        is_training=False)

    sess = tf.get_default_session()
    if scope is None:
        sess.run(tf.initialize_all_variables())

    o = sess.run(output[-1, :, :])
    return o

def _testTfRnn(input_data, num_layers, num_units, input_size, batch_size, seed, scope=None):
    inputs = tf.unpack(input_data)
    #init = tf.uniform_unit_scaling_initializer(seed=seed)
    init = tf.random_uniform_initializer(-0.1, 0.1, seed=seed)
    #init = tf.constant_initializer(1.0)

    cell = tf.nn.rnn_cell.LSTMCell(num_units=num_units, initializer=init, state_is_tuple=True, forget_bias=0.0)
    cell = tf.nn.rnn_cell.MultiRNNCell([cell] * num_layers)

    input_h = tf.zeros([batch_size, num_units])
    input_c = tf.zeros([batch_size, num_units])
    output, output_c = tf.nn.rnn(cell, inputs, initial_state=[(input_c, input_h)] * num_layers, dtype=tf.float32, scope=scope)

    sess = tf.get_default_session()
    if scope is None:
        sess.run(tf.initialize_all_variables())

    o = sess.run(output)[-1]
    return o

def _print_diff(o1, o2):
    import math
    delta2 = (o1 - o2)**2
    err = {}
    err['rms'] = math.sqrt(tf.reduce_mean(delta2).eval())
    err['max'] = math.sqrt(tf.reduce_max(delta2).eval())
    err['min'] = math.sqrt(tf.reduce_min(delta2).eval())
    print('errs: min={min} rms={rms} max={max}'.format(**err))

if __name__ == '__main__':
    num_layers = 4
    #"""
    num_units = 32
    input_size = 128
    seq_length = 31
    batch_size = 60
    """
    num_units = 2
    input_size = 2
    seq_length = 2
    batch_size = 2
    #"""

    config = tf.ConfigProto()
    config.log_device_placement = False
    config.gpu_options.allow_growth = True

    import sys
    num_tests = 1
    with tf.Session(config=config) as sess:
        """
        input_data = tf.Variable(tf.random_uniform([seq_length, batch_size, input_size], -1.0, 1.0, seed=0), trainable=False)
        sess.run(tf.initialize_variables([input_data]))
        print input_data.eval()
        """
        input_data = tf.reshape(tf.cast(tf.range(seq_length * batch_size * input_size) + 1, tf.float32), [seq_length, batch_size, input_size]) / (seq_length * batch_size * input_size / 10.0)
        #input_data = tf.zeros([seq_length, batch_size, input_size])
        #input_data = tf.ones([seq_length, batch_size, input_size]) * -0.79827476
        #print input_data.eval()
        '''
        res = None
        for test in xrange(num_tests):
            first_run = test == 0
            with tf.variable_scope('test_scope') as vs:
                if not first_run:
                    vs.reuse_variables()
                #"""
                o1 = _testCudnn(input_data, num_layers, num_units, input_size, batch_size, (test + 123) * num_tests)
                o2 = _testTfRnn(input_data, num_layers, num_units, input_size, batch_size, (test + 987) * num_tests)
                """
                o1 = _testTfRnn(input_data, num_layers, num_units, input_size, batch_size, (test + 123) * num_tests)
                o2 = _testTfRnn(input_data, num_layers, num_units, input_size, batch_size, (test + 987) * num_tests)
                """
                if res is None:
                    res = o1
                else:
                    res = res + o1
                res = res - o2
            print >> sys.stderr, (test + 1) * 100.0 / num_tests, '% done\r',

        print >> sys.stderr
        print sess.run(tf.reduce_mean(tf.reshape(res / num_tests, [-1])**2, 0))**0.5
        '''

        with tf.variable_scope('load_weights_test') as vs:
            w = tf.uniform_unit_scaling_initializer(seed=0)
            #def w(shape, dtype=tf.float32, partition_info=None):
            #    num_elements = reduce(lambda x, y: x * y, shape)
            #    return tf.reshape(tf.cast(tf.range(num_elements), dtype) + 1, shape) / num_elements
            b = tf.constant_initializer(1.0)
            model = CanonicalCudnnLstm(num_layers, num_units, input_size, w, b, forget_bias=0.0)
            sess.run(tf.global_variables_initializer())
            params = model.get_canonical_weights()
            """
            print params
            for layer, params_dict in enumerate(params):
                print layer
                for name, matrix in sorted(params_dict.items(), key=lambda x: (-len(x[0]), x[0].endswith('biases'), x[0])):
                    print '\t', name, matrix.eval()
            """
            #print model.params.eval().shape, model.params.eval()

            for layer in xrange(num_layers):
                with tf.variable_scope('MultiRNNCell/Cell{}/LSTMCell'.format(layer)) as lstm_vs:
                    params_dict = params[layer]
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
                    init = tf.variables_initializer([W, B])
                    sess.run(init)
                    #print W.eval().shape, '\n', W.eval()
                    #print B.eval().shape, '\n', B.eval()

            vs.reuse_variables()
            o1 = _testCudnn(input_data, num_layers, num_units, input_size, batch_size, 123 * num_tests, vs)
            o2 = _testTfRnn(input_data, num_layers, num_units, input_size, batch_size, 987 * num_tests, vs)

            print('o1: {} {} {}'.format(o1.shape, o1.flatten()[:5], o1.flatten()[-5:]))
            print('o2: {} {} {}'.format(o2.shape, o2.flatten()[:5], o2.flatten()[-5:]))
            _print_diff(o1, o2)

        for v in tf.global_variables():
            print v.name

