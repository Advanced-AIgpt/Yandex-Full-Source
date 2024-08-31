# -*- coding: utf-8 -*-

import tensorflow as tf

from six.moves import zip


class TfModelApplier(object):
    def __init__(self, path, input_names, output_names):
        with tf.gfile.GFile(path, 'rb') as f:
            graph_def = tf.GraphDef()
            graph_def.ParseFromString(f.read())

        with tf.Graph().as_default() as graph:
            self._sess = tf.Session(graph=graph)

            return_elements = tf.import_graph_def(
                graph_def, return_elements=input_names + output_names
            )

            self._input_nodes = {name: op for op, name in zip(return_elements, input_names)}
            self._output_nodes = [op for op in return_elements[len(input_names):]]

    def __call__(self, inputs, output_node_indices=None):
        assert inputs.keys() == self._input_nodes.keys()
        inputs = {self._input_nodes[key]: inputs[key] for key in inputs.keys()}

        output_nodes = self._output_nodes
        if output_node_indices is not None:
            output_nodes = [self._output_nodes[index] for index in output_node_indices]
        return self._sess.run(output_nodes, feed_dict=inputs)
