import os
import tensorflow as tf
import numpy as np
from alice.boltalka.py_libs.apply_nlg_dssm.apply_nlg_dssm import NlgDssmApplier
from alice.boltalka.extsearch.query_basesearch.lib.main import QueryBasesearch

def load_graph(frozen_graph_filename):
    # We load the protobuf file from the disk and parse it to retrieve the
    # unserialized graph_def
    with tf.gfile.GFile(frozen_graph_filename, "rb") as f:
        graph_def = tf.GraphDef()
        graph_def.ParseFromString(f.read())

    # Then, we import the graph_def into a new Graph and returns it
    with tf.Graph().as_default() as graph:
        # The name var will prefix every op/nodes in your graph
        # Since we load everything in a new graph, this is not needed
        tf.import_graph_def(graph_def)
    return graph

EMOJIS = ['other', 'laugh', 'love', 'happy', 'wink', 'smirk', 'cry', 'sweat', 'think', 'thum', 'cool', 'facepalm']

class BoltalkaEmojifier:
    def __init__(self, data_dir='.'):
        self.graph = load_graph(os.path.join(data_dir, 'data', 'model.pb'))
        self.session = tf.Session(graph=self.graph)
        self.applier = NlgDssmApplier(os.path.join(data_dir, 'data', 'model'))
        self.searcher = QueryBasesearch(max_results=10)
    
    def predict_emoji(self, context, reply):
        embeddings = self.applier.get_embeddings([context], [reply])
        embeddings = [embeddings[0] + embeddings[1]]
        y = self.graph.get_tensor_by_name('import/add_4:0')
        pred = self.session.run(y, feed_dict={
            'import/input.1:0': embeddings
        })
        return EMOJIS[np.argmax(pred)]
    
    def get_replies_with_emoji(self, context):
        candidates = [el[0] for el in self.searcher.get_replies(context)]
        candidates = [(candidate, self.predict_emoji(context, candidate)) for candidate in candidates]
        return candidates
