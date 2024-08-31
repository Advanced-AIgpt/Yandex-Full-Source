import sys
sys.path.append('distributed_self_play/')
from train_classifier import Experiment

import torch

experiment = Experiment.from_cmdline()
experiment.model.cpu()
torch.onnx.export(experiment.model, torch.FloatTensor([[0]*600]*1), "model.onnx")
import onnx
from onnx_tf.backend import prepare
onnx_model = onnx.load("model.onnx")
print(onnx.helper.printable_graph(onnx_model.graph))
onnx_model.graph.input[0].type.tensor_type.shape.dim[0].dim_param = "?"
print(onnx_model.graph.output)
tf_rep = prepare(onnx_model)
tf_rep.export_graph("model.pb")
