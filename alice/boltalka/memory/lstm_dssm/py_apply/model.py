import tensorflow as tf


def load_pb(path_to_pb):
    with tf.gfile.GFile(path_to_pb, 'rb') as f:
        graph_def = tf.GraphDef()
        graph_def.ParseFromString(f.read())
    with tf.Graph().as_default() as graph:
        tf.import_graph_def(graph_def, name='')
        return graph


class Model:
    def __init__(self, encoder_path, dssm_path):
        self._lstm_graph = load_pb(encoder_path)
        self._dssm_graph = load_pb(dssm_path)

    def apply(self, prev_reply_embedding, context_embedding, reply_embeddings, state):
        new_state_tensor = self._lstm_graph.get_tensor_by_name('output:0')
        input_tensor = self._lstm_graph.get_tensor_by_name('input:0')
        h_tensor = self._lstm_graph.get_tensor_by_name('h:0')
        c_tensor = self._lstm_graph.get_tensor_by_name('c:0')

        if prev_reply_embedding is not None:
            with tf.Session(graph=self._lstm_graph) as sess:
                state = sess.run(new_state_tensor, feed_dict={
                                 input_tensor: prev_reply_embedding, h_tensor: state[:, 0], c_tensor: state[:, 1]})
        with tf.Session(graph=self._lstm_graph) as sess:
            state = sess.run(new_state_tensor, feed_dict={
                             input_tensor: context_embedding, h_tensor: state[:, 0], c_tensor: state[:, 1]})
        score_tensor = self._dssm_graph.get_tensor_by_name('Squeeze:0')
        context_tensor = self._dssm_graph.get_tensor_by_name('context:0')
        reply_tensor = self._dssm_graph.get_tensor_by_name('reply:0')
        # in fact batch_size > 1 is not supported, but you can send as many candidates as you wish
        with tf.Session(graph=self._dssm_graph) as sess:
            scores = sess.run(score_tensor, feed_dict={
                              context_tensor: state[0, 1], reply_tensor: reply_embeddings})
        return (scores, state)
