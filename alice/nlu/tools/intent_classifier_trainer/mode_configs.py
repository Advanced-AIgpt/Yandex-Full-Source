import json
import os
import logging
import hashlib

class Config:
    @staticmethod
    def _hash_path(path):
        return hashlib.md5(path).hexdigest()

    def __init__(
                self, 
                mode=None, 
                model_path=None, 
                positives_path=None, 
                negatives_path=None, 
                embeddings_path=None, 
                train_config=None, 
                result_path=None, 
                threshold=0.5,
                embedding="dssm",
                **args):
        assert(mode == 'fit' or mode == 'predict')

        if positives_path is not None and not os.path.exists(positives_path):
            raise ValueError('Expected tsv file at {}'.format(positives_path))

        if negatives_path is not None and not os.path.exists(negatives_path):
            raise ValueError('Expected tsv file at {}'.format(negatives_path))

        if mode == 'predict' and not os.path.isdir(model_path):
            raise ValueError('In predict mode expected directory with pretrained model at {}.'.format(model_path))

        self.mode = mode
        self.positives_path = positives_path
        self.negatives_path = negatives_path
        self.embeddings_path = embeddings_path

        self.model_path = model_path
        if train_config is not None:
            self.hidden_layer_sizes = train_config['hidden_layer_sizes']
            self.batch_size = train_config['batch_size']
            self.epoch_count = train_config['epoch_count']
            
        self.result_path = result_path
        self.input_vector_size = None
        self.threshold = threshold

        if embedding != "dssm" and embedding != "bert":
            raise ValueError('Expected dssm or bert embeddings, found embedding={}'.format(embedding))

        self.embeddings_src = embedding
    
    def get_train_config(self):
        return {
            'hidden_layer_sizes': self.hidden_layer_sizes,
            'batch_size': self.batch_size,
            'epoch_count': self.epoch_count,
            'input_vector_size': self.input_vector_size,
            'threshold': self.threshold
        }

    def set_train_config(self, train_config):
        self.hidden_layer_sizes = train_config['hidden_layer_sizes']
        self.batch_size = train_config['batch_size']
        self.epoch_count = train_config['epoch_count']
        self.input_vector_size = train_config['input_vector_size']
        self.threshold = train_config['threshold']

    @classmethod
    def from_json(cls, filepath):
        logging.info("Opening config to model on {}".format(filepath))
        with open(filepath, 'r') as input_file:
            data = json.load(input_file)

            data['model_path'] = data['model_path'] + '/' # in case someone forgets it

            def safe_change_path(data, pathname):
                if pathname in data and not os.path.exists(pathname):
                    #pass
                    data[pathname] = os.path.normpath(os.path.dirname(filepath) + '/' + data[pathname])

            safe_change_path(data, 'model_path')
            safe_change_path(data, 'embeddings_path')
            safe_change_path(data, 'result_path')
            safe_change_path(data, 'positives_path')
            safe_change_path(data, 'negatives_path')
            
            return cls(**data)
