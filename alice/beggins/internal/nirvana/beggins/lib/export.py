import json
import os

import attr
import tensorflow as tf
from tensorflow.python.framework import graph_io

from beggins.lib import config, container


@attr.s(frozen=True)
class FrozenModelInfo:
    input_node = attr.ib()
    output_node = attr.ib()
    input_shape = attr.ib()

    def to_json(self):
        obj = {
            'input_node': f'{self.input_node}',
            'output_node': f'{self.output_node}:0',
            'input_vector_size': self.input_shape,
        }
        return json.dumps(obj, sort_keys=True, indent=4)


def freeze_graph(graph, session, output, output_filepath):
    with graph.as_default():
        graph_def_inf = tf.graph_util.remove_training_nodes(graph.as_graph_def())
        graph_def_frozen = tf.graph_util.convert_variables_to_constants(session, graph_def_inf, output)
        graph_io.write_graph(graph_def_frozen, os.path.dirname(output_filepath), os.path.basename(output_filepath),
                             as_text=False)
        return graph_def_frozen


def freeze_model(input_filepath, output_filepath):
    tf.keras.backend.set_learning_phase(0)
    base_model = tf.keras.models.load_model(input_filepath)
    session = tf.keras.backend.get_session()
    input_node_name = base_model.inputs[0].op.name
    output_node_name = base_model.outputs[0].op.name
    shape = int(base_model.inputs[0].op.values()[0].shape[1])
    _ = freeze_graph(session.graph, session, [out.op.name for out in base_model.outputs], output_filepath)
    return FrozenModelInfo(input_node_name, output_node_name, shape)


def export(cfg: config.Export):
    def get_paths(model_name):
        folder = os.path.join(container.OUTPUT.data_file('export'), model_name)
        os.makedirs(folder, exist_ok=True)
        return os.path.join(folder, 'model.pb'), os.path.join(folder, 'model_description.json')

    try:
        for model in cfg.models:
            tf.keras.backend.clear_session()
            model_path, model_description_path = get_paths(model.name)
            model_info = freeze_model(model.filename, model_path)
            with open(model_description_path, 'w') as f:
                f.write(model_info.to_json())
    finally:
        tf.keras.backend.clear_session()
