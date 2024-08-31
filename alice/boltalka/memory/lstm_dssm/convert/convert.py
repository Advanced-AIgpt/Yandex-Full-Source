# python 3.6
import argparse
import torch
import torch.nn as nn
import torch.nn.functional as F
import collections
import onnx
import tensorflow as tf
from onnx_tf.backend import prepare


class LSTMCellModule(nn.Module):
    def __init__(self, input_size=300, hidden_size=256):
        super(LSTMCellModule, self).__init__()

        self.lstm = nn.LSTMCell(input_size=input_size, hidden_size=hidden_size)

    def forward(self, input, h, c):
        h_n, c_n = self.lstm(input, (h, c))
        return torch.cat((torch.unsqueeze(h_n, 1), torch.unsqueeze(c_n, 1)), dim=1)


class DSSMModule(nn.Module):
    def __init__(self, context_size=256, reply_size=300):
        super(DSSMModule, self).__init__()
        self.linear = nn.Linear(context_size, reply_size)
        self.reply_encoder = nn.Sequential(nn.Linear(reply_size, reply_size), nn.ReLU(
        ), nn.Linear(reply_size, reply_size), nn.ReLU(), nn.Linear(reply_size, reply_size))
        self.final_change = nn.Linear(1, 1)

    def forward(self, context, reply):
        context = torch.unsqueeze(context, 0)
        output = self.linear(context)

        rel_embeds = F.normalize(self.reply_encoder(reply), dim=1)
        output = F.normalize(output, dim=1)

        scores = (rel_embeds * output).sum(1)
        scores = self.final_change(scores.unsqueeze(1)).squeeze(1)

        return scores


def convert(filename):
    model_onnx = onnx.load(f'{filename}.onnx')
    prepare(model_onnx).export_graph(f'{filename}.pb')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--experiment', required=True)
    parser.add_argument('--lstm-name', default="lstm")
    parser.add_argument('--dssm-name', default="dssm")
    args = parser.parse_args()

    model_weights = torch.load(args.experiment, map_location={
                               'cuda:0': 'cpu'})['states']['model']
    lstm_weights = collections.OrderedDict()
    for x in model_weights:
        if x.startswith("lstm"):
            lstm_weights[x] = model_weights[x]
    dssm_weights = collections.OrderedDict()
    dssm_model = DSSMModule()
    for x in dssm_model.state_dict().keys():
        dssm_weights[x] = model_weights[x]

    cell_model = LSTMCellModule()
    dssm_model.load_state_dict(dssm_weights)
    new_dict = collections.OrderedDict(
        [(k, lstm_weights["%s_l%d" % (k, 0)]) for k in cell_model.state_dict().keys()])
    cell_model.load_state_dict(new_dict)

    torch.onnx.export(cell_model, (torch.rand(1, 300), torch.rand(1, 256), torch.rand(1, 256)), f"{args.lstm_name}.onnx", input_names=['input', 'h', 'c'], output_names=['output'],
                      dynamic_axes={'input': {0: 'batch_size'},
                                    'h': {0: 'batch_size'},
                                    'c': {0: 'batch_size'},
                                    'output': {0: 'batch_size'}}, verbose=True)
    torch.onnx.export(dssm_model, (torch.rand(256), torch.rand(1, 300)), f"{args.dssm_name}.onnx", input_names=['context', 'reply'], output_names=['output'],
                      dynamic_axes={
        'reply': {0: 'reply_size'},
        'output': {0: 'reply_size'}}, verbose=True)

    convert(args.lstm_name)
    convert(args.dssm_name)
