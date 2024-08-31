import os
import numpy as np
import tensorflow as tf

import data_utils
from cudnn_wrapper import CanonicalCudnnLstm as CudnnLstm
from cudnn_to_tf import cudnn_to_tf_lstm

def _have_gpu():
    me = _have_gpu
    if not hasattr(me, 'answer'):
        from tensorflow.python.client import device_lib # undocumented
        devices = device_lib.list_local_devices()
        gpus = [d for d in devices if d.device_type == 'GPU']
        me.answer = True if gpus else False
    return me.answer

class Saver_stub(object):
    def __init__(self, global_variables, fixlist):
        self.model_variables = global_variables
        self.fixlist = fixlist
    def save(self):
        raise Exception('This saver can only restore model!')
    def restore(self, sess, path):
        from tensorflow.python import pywrap_tensorflow as pytf
        reader = pytf.NewCheckpointReader(path)
        reader_variables = reader.get_variable_to_shape_map()
        todo_variables = []
        for var in self.model_variables:
            name = var.name.replace(':0','')
            if name in reader_variables:
                val = reader.get_tensor(name)
                sess.run(var._initializer_op, {var._initializer_op.inputs[1]: val})
                del(reader_variables[name])
            else:
                todo_variables.append(var)
        for fix in self.fixlist:
            fix(sess)

class SemiCudnnSeq2SeqModel(object):
    def __init__(self,
                 dct,
                 embedding_size,
                 lstm_size,
                 num_layers,
                 max_gradient_norm,
                 max_input_sequence_length,
                 max_output_sequence_length,
                 decode=False,
                 temperature=1.0,
                 softmax_num_samples=0,
                 optimizer=None):

        self.dct = dct
        dict_size = len(dct)

        self.max_input_sequence_length = max_input_sequence_length
        self.max_output_sequence_length = max_output_sequence_length

        self.decode = decode

        with tf.variable_scope('semi_cudnn_seq2seq', dtype=tf.float32):

            self.global_step = tf.get_variable('global_step',
                                               shape=[],
                                               initializer=tf.constant_initializer(0),
                                               trainable=False,
                                               dtype=tf.int32)

            self.embeddings = tf.get_variable('embeddings',
                                              shape=[dict_size, embedding_size],
                                              initializer=tf.uniform_unit_scaling_initializer())

            with tf.variable_scope('encoder'):
                # sequence_length x batch_size
                self.encoder_inputs = tf.placeholder(tf.int32, shape=[max_input_sequence_length, None], name='inputs')
                # sequence_length x batch_size x embedding_size
                self.encoder_input_data = tf.gather(self.embeddings, self.encoder_inputs)
                self.sequences_lengths = tf.placeholder(tf.int32, shape=[None], name='sequences_lengths')

                self.encoder = CudnnLstm(num_layers=num_layers,
                                         num_units=lstm_size,
                                         input_size=embedding_size,
                                         weights_initializer=tf.uniform_unit_scaling_initializer(),
                                         biases_initializer=tf.constant_initializer(0.0),
                                         forget_bias=0.0)

                self.encoder_outputs = self.encoder(input_data=self.encoder_input_data,
                                                    sequences_lengths=self.sequences_lengths)

            with tf.variable_scope('decoder') as decoder_vs:
                # sequence_length x batch_size
                self.decoder_inputs = tf.placeholder(tf.int32, shape=[max_output_sequence_length + 1, None], name='inputs')
                self.decoder_sequences_lengths = tf.placeholder(tf.int32, shape=[None], name='sequences_lengths')
                self.target_weights = tf.placeholder(tf.float32, shape=[max_output_sequence_length, None], name='target_weights')
                self.targets = self.decoder_inputs[1:, :]

                self.sampled_decoder_outputs = tf.get_variable(name='sampled_outputs',
                                                               shape=[max_output_sequence_length],
                                                               initializer=tf.constant_initializer(-1),
                                                               trainable=False,
                                                               dtype=tf.int64)


                # sequence_length x batch_size x embedding_size
                self.decoder_input_data = tf.concat(2,
                    [
                        tf.gather(self.embeddings, self.decoder_inputs[:-1, :]),
                        tf.tile(tf.expand_dims(self.encoder_outputs, 0), [max_output_sequence_length, 1, 1])
                    ])

                cell = tf.nn.rnn_cell.LSTMCell(num_units=lstm_size,
                                               initializer=tf.uniform_unit_scaling_initializer(),
                                               state_is_tuple=True,
                                               forget_bias=0.0)
                self.decoder = tf.nn.rnn_cell.MultiRNNCell([cell] * num_layers)

                intermediate_projection_size = lstm_size // 4
                with tf.variable_scope('intermediate_projection'):
                    self.intermediate_projection_weights = tf.get_variable('weights',
                                                                           shape=[lstm_size, intermediate_projection_size],
                                                                           initializer=tf.uniform_unit_scaling_initializer())
                    self.intermediate_projection_biases = tf.get_variable('biases',
                                                                          shape=[intermediate_projection_size],
                                                                          initializer=tf.constant_initializer(0.0))

                with tf.variable_scope('output_projection'):
                    self.output_projection_weights_t = tf.get_variable('weights',
                                                                       shape=[dict_size, intermediate_projection_size],
                                                                       initializer=tf.uniform_unit_scaling_initializer())
                    self.output_projection_biases = tf.get_variable('biases',
                                                                    shape=[dict_size],
                                                                    initializer=tf.constant_initializer(0.0))
                batch_size = tf.shape(self.encoder_inputs)[1]
                if not decode:
                    self.decoder_outputs, _ = tf.nn.seq2seq.rnn_decoder(decoder_inputs=tf.unpack(self.decoder_input_data),
                                                                        initial_state=self.decoder.zero_state(batch_size, tf.float32),
                                                                        cell=self.decoder,
                                                                        scope=decoder_vs)
                    self.decoder_outputs = tf.pack(self.decoder_outputs)
                    with tf.variable_scope('intermediate_projection'):
                        batched_decoder_outputs = tf.transpose(self.decoder_outputs, perm=[1, 0, 2])
                        batched_weights = tf.tile(tf.expand_dims(self.intermediate_projection_weights, 0), [batch_size, 1, 1])
                        batched_biases = tf.tile(tf.expand_dims(tf.expand_dims(self.intermediate_projection_biases, 0), 0), [batch_size, max_output_sequence_length, 1])
                        self.intermediate_projection = tf.tanh(
                            tf.batch_matmul(batched_decoder_outputs, batched_weights) + batched_biases)

                    with tf.variable_scope('output_projection'):
                        batched_weights = tf.tile(tf.expand_dims(tf.transpose(self.output_projection_weights_t), 0), [batch_size, 1, 1])
                        batched_biases = tf.tile(tf.expand_dims(tf.expand_dims(self.output_projection_biases, 0), 0), [batch_size, max_output_sequence_length, 1])
                        # batch_size x sequence_length x dict_size
                        self.output_projection = tf.batch_matmul(self.intermediate_projection, batched_weights) + batched_biases

                else:
                    unk = self.dct.get(data_utils._UNK, None)
                    self.last_save_token_id_op = None
                    # TODO(alipov): support batch_size > 1
                    def loop_function(prev, timestep):
                        intermediate_projection = tf.tanh(
                            tf.matmul(prev, self.intermediate_projection_weights) + self.intermediate_projection_biases)
                        output_projection = tf.matmul(intermediate_projection, tf.transpose(self.output_projection_weights_t)) \
                            + self.output_projection_biases
                        probs = tf.nn.softmax(output_projection)

                        if unk is not None:
                            probs = probs * (1.0 - tf.one_hot([unk], dict_size))
                            probs = probs / tf.reduce_sum(probs)
                            probs = probs**(1.0 / temperature)
                            probs = probs / tf.reduce_sum(probs)

                        probs = tf.cumsum(probs, 1)
                        p = tf.random_uniform(shape=[])
                        probs = tf.cast(probs < p, tf.int32)
                        token_id = tf.argmin(probs, 1)
                        with tf.control_dependencies([self.last_save_token_id_op] if self.last_save_token_id_op is not None else None):
                            save_token_id_op = self.sampled_decoder_outputs[timestep - 1].assign(tf.squeeze(token_id))
                            self.last_save_token_id_op = save_token_id_op
                        return tf.concat(1, [tf.gather(self.embeddings, token_id), self.encoder_outputs])

                    self.decoder_outputs, _ = tf.nn.seq2seq.rnn_decoder(decoder_inputs=tf.unpack(self.decoder_input_data),
                                                                        initial_state=self.decoder.zero_state(batch_size, tf.float32),
                                                                        cell=self.decoder,
                                                                        loop_function=loop_function,
                                                                        scope=decoder_vs)
                    # saving last token
                    loop_function(self.decoder_outputs[-1], self.max_output_sequence_length)

                    self.decoder_outputs = tf.pack(self.decoder_outputs)

            if not decode:
                with tf.name_scope('loss') as ns:
                    self.loss = tf.nn.sparse_softmax_cross_entropy_with_logits(self.output_projection, tf.transpose(self.targets))
                    self.loss = tf.reduce_mean(tf.reduce_sum(self.loss * tf.transpose(self.target_weights), 1))

                    self.sampled_loss = None
                    if softmax_num_samples > 0:
                        self.sampled_loss = tf.nn.sampled_softmax_loss(
                            self.output_projection_weights_t,
                            self.output_projection_biases,
                            tf.reshape(self.intermediate_projection, [-1, intermediate_projection_size]),
                            tf.reshape(tf.transpose(self.targets), [-1, 1]),
                            softmax_num_samples,
                            dict_size)
                        self.sampled_loss = tf.reshape(self.sampled_loss, [batch_size, -1])
                        self.sampled_loss = tf.reduce_mean(tf.reduce_sum(self.sampled_loss * tf.transpose(self.target_weights), 1))

                if not optimizer: optimizer = tf.train.AdamOptimizer()

                with tf.variable_scope('gradients'):
                    self.optimizer = optimizer
                    self.gradients = self.optimizer.compute_gradients(self._get_training_loss())
                    for i, (grad, var) in enumerate(self.gradients):
                        if grad is not None:
                            self.gradients[i] = (tf.clip_by_norm(grad, max_gradient_norm), var)
                    self.train_op = self.optimizer.apply_gradients(self.gradients, global_step=self.global_step)

                if _have_gpu():
                    self.saver = tf.train.Saver(tf.global_variables())
                else:
                    self.saver = Saver_stub(tf.global_variables(), [
                        lambda sess: self.encoder.set_tf_vars(sess)
                        ])

    def _get_training_loss(self):
        return self.loss if self.sampled_loss is None else self.sampled_loss

    def _pad(self, data, length, pad):
        return data +  [pad] * (length - len(data))

    def _reshape_as_time_major(self, data, dtype):
        return np.array(data, dtype=dtype).T

    def _get_batch(self, input_data):
        pad = self.dct[data_utils._PAD]
        go = self.dct[data_utils._GO]

        encoder_inputs = []
        sequences_lengths = []
        decoder_inputs = []
        target_weights = []

        for context, reply in input_data:
            context = context[-self.max_input_sequence_length:]
            encoder_inputs.append(self._pad(context, self.max_input_sequence_length, pad))
            sequences_lengths.append(len(context))

            reply = reply[:self.max_output_sequence_length]
            decoder_inputs.append([go] + self._pad(reply, self.max_output_sequence_length, pad))
            target_weights.append(self._pad([1] * len(reply), self.max_output_sequence_length, 0))

        encoder_inputs = self._reshape_as_time_major(encoder_inputs, np.int32)
        sequences_lengths = np.array(sequences_lengths, dtype=np.int32)
        decoder_inputs = self._reshape_as_time_major(decoder_inputs, np.int32)
        target_weights = self._reshape_as_time_major(target_weights, np.float32)

        return encoder_inputs, sequences_lengths, decoder_inputs, target_weights

    def __call__(self, sess, input_data, is_training=True):
        encoder_inputs, sequences_lengths, decoder_inputs, target_weights = self._get_batch(input_data)

        input_feed = {}
        input_feed[self.encoder_inputs.name] = encoder_inputs
        input_feed[self.sequences_lengths.name] = sequences_lengths
        input_feed[self.decoder_inputs.name] = decoder_inputs
        input_feed[self.target_weights.name] = target_weights

        output_feed = []
        if is_training:
            output_feed.append(self._get_training_loss())
            output_feed.append(self.train_op)
        else:
            if self.decode:
                output_feed.append(self.last_save_token_id_op)
                output_feed.append(self.sampled_decoder_outputs)
            else:
                output_feed.append(self.loss)

        return sess.run(output_feed, input_feed)

    @staticmethod
    def load_from_cudnn_seq2seq(sess, cudnn_model):
        if hasattr(cudnn_model, 'models'):
            cudnn_model = cudnn_model.models[0]
        params = []
        with tf.variable_scope('semi_cudnn_seq2seq'):
            #v = tf.get_variable(name='learning_rate', initializer=cudnn_model.learning_rate)
            #params.append(v)
            v = tf.get_variable(name='global_step', initializer=cudnn_model.global_step)
            params.append(v)
            v = tf.get_variable(name='embeddings', initializer=cudnn_model.embeddings)
            params.append(v)
            with tf.variable_scope('encoder'):
                v = tf.get_variable(name='cudnn_lstm_params', initializer=cudnn_model.encoder.params)
                params.append(v)
            with tf.variable_scope('decoder') as decoder_vs:
                v = tf.get_variable(name='sampled_outputs',
                                    shape=[cudnn_model.max_output_sequence_length],
                                    initializer=tf.constant_initializer(-1),
                                    trainable=False,
                                    dtype=tf.int64)
                params.append(v)
                cudnn_to_tf_lstm(sess, decoder_vs, cudnn_model.decoder)
                with tf.variable_scope('intermediate_projection'):
                    v = tf.get_variable(name='weights', initializer=cudnn_model.intermediate_projection_weights)
                    params.append(v)
                    v = tf.get_variable(name='biases', initializer=cudnn_model.intermediate_projection_biases)
                    params.append(v)
                with tf.variable_scope('output_projection'):
                    v = tf.get_variable(name='weights', initializer=cudnn_model.output_projection_weights_t)
                    params.append(v)
                    v = tf.get_variable(name='biases', initializer=cudnn_model.output_projection_biases)
                    params.append(v)
        sess.run(tf.initialize_variables(params))

if __name__ == '__main__':
    with open('empty.dict', 'w') as out:
        pass
    from data_utils import *
    dct, _ = load_dictionary('empty.dict')

    with tf.variable_scope('model') as vs:
        model = SemiCudnnSeq2SeqModel(dct=dct,
                                      embedding_size=10,
                                      lstm_size=33,
                                      num_layers=2,
                                      max_gradient_norm=0.9,
                                      max_input_sequence_length=5,
                                      max_output_sequence_length=6,
                                      softmax_num_samples=0)
        vs.reuse_variables()
        decoder = SemiCudnnSeq2SeqModel(dct=dct,
                                        embedding_size=10,
                                        lstm_size=33,
                                        num_layers=2,
                                        max_gradient_norm=0.9,
                                        max_input_sequence_length=5,
                                        max_output_sequence_length=6,
                                        softmax_num_samples=0,
                                        decode=True,
                                        temperature=0.1)

    print 'Created model!'
    config = tf.ConfigProto()
    config.log_device_placement = False
    config.gpu_options.allow_growth = True

    input_data = [
        [[3, 3, 3, 2], [3, 2]],
        [[3, 3, 3, 3, 2], [3, 3, 3, 3, 2]]
    ]

    with tf.Session(config=config) as sess:
        sess.run(tf.global_variables_initializer())
        #for v in tf.all_variables(): print( v.name, v.eval().tolist() )
        prefix = './fox'
        if _have_gpu():
            if not os.path.exists(prefix + '.meta'):
                print('TRAIN ON GPU')
                for train_it in xrange(100):
                    loss, _ = model(sess, input_data)
                print train_it, loss # print only last iteration
                print('SAVE GPU TRAINED MODEL')
                model.saver.save(sess, prefix)
            print('RESTORE GPU TRAINED MODEL into GPU MODEL')
            model.saver.restore(sess, prefix)
        else:
            if not os.path.exists(prefix + '.meta'):
                print('YOU SHOULD TRAIN YOUR MODEL ON GPU FIRST')
                exit(0)
            print('RESTORE GPU TRAINED MODEL into HOST MODEL')
            model.saver.restore(sess, prefix)

        print model(sess, input_data, is_training=False)

        _, sampled = decoder(sess, [[[3, 3, 3, 2], []]], is_training=False)
        print sampled
        _, sampled = decoder(sess, [[[3, 3, 3, 3, 2], []]], is_training=False)
        print sampled

        print('HERE ARE SOME MORE DIGITS FOR ENCODER ' + str(model.encoder.model))
        encoder_inputs, sequences_lengths, decoder_inputs, target_weights = model._get_batch(input_data)
        feeder = {
            model.encoder_inputs.name : encoder_inputs,
            model.sequences_lengths.name : sequences_lengths,
            model.decoder_inputs.name : decoder_inputs,
            model.target_weights.name : target_weights,
            }
        encoder_name_space = model.encoder.params.name.replace('/cudnn_lstm_params:0', '')
        for v in [model.encoder.params.name,
                  model.encoder.params.name.replace('/cudnn_lstm_params:',  '/one_hot:'),
                  model.encoder.params.name.replace('/cudnn_lstm_params:',  '/Sum:'),
            ]:
            res = sess.run(v, feeder).flatten().tolist()
            print(v, res[:3] + ['...'] + res[-3:])

        if not _have_gpu():
            print('SAVE GRAPH of the model')
            out_ops = [tensor.op.name for tensor in [
                model.targets,
                model.encoder_outputs,
                model.decoder_outputs,
                model.decoder_sequences_lengths,
                model.intermediate_projection,
                model.output_projection,
                model.sampled_decoder_outputs,
                # Remaining operations to keep in graph are for verificaton
                model.encoder_inputs,
                model.decoder_inputs,
                model.target_weights,
                model.sequences_lengths,
                model.encoder.params,
                model.loss,
                ]]
            gd = sess.graph.as_graph_def()
            print('{n} nodes in original graph_def'.format(n=len(gd.node)))
            gd = tf.graph_util.remove_training_nodes(gd)
            print('{n} nodes after removal of training nodes'.format(n=len(gd.node)))
            gd = tf.graph_util.convert_variables_to_constants(sess, gd, out_ops)
            print('{n} nodes after converting variables to constants'.format(n=len(gd.node)))
            with open(prefix + '.graph', 'wb') as f:
                f.write(gd.SerializeToString())
                print('SAVED {}'.format(prefix + '.graph'))
