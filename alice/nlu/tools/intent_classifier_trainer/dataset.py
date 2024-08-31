import logging
import json
import base64
import numpy as np
import os
import time
import pandas as pd


class Dataset:
    def __init__(self, config):
        self.config = config
        self.samples = {'pos': None, 'neg': None}
        self.pos, self.samples['pos'], self.pos_weights = self.read_tsv_file(config.positives_path, is_pos=True)
        self.neg, self.samples['neg'], self.neg_weights = self.read_tsv_file(config.negatives_path, is_pos=False)

        if self.neg.size != 0:
            config.input_vector_size = self.neg.shape[1]
        elif self.pos.size != 0:
            config.input_vector_size = self.pos.shape[1]
        else:
            raise ValueError('Expected to have at least one of positives / negatives datasets')

        if self.pos.size == 0:
            self.pos = self.pos.reshape((0, config.input_vector_size))
        elif self.neg.size == 0:
            self.neg = self.neg.reshape((0, config.input_vector_size))

        if self.pos.shape[0] != self.pos_weights.shape[0]:
            raise ValueError('Negative samples and weights should have same number of elements')

        if self.neg.shape[0] != self.neg_weights.shape[0]:
            raise ValueError('Negative samples and weights should have same number of elements')

        if self.pos.shape[1] != self.neg.shape[1]:
            raise ValueError('Vectors of pos and neg samples hould have same dimension')
    

    def normalize_weights(self):
        if self.pos_weights.size != 0:
            self.pos_weights = self.pos_weights / self.pos_weights.sum()
        if self.neg_weights.size != 0:
            self.neg_weights = self.neg_weights / self.neg_weights.sum()

    def read_tsv_file(self, filename, is_pos, need_shuffle=True):
        logging.info('Preparing file %s' % filename)
        '''returns embeddings, samples, weights'''
        if filename is None:
            return np.array([]), [], np.array([])

        dataframe = pd.read_csv(filename, sep="\t")
        if 'text' not in dataframe:
            raise ValueError('Expected text column with samples in {}'.format(filename))
        
        if not is_pos:
            dataframe['not_in_pos'] = dataframe['text'].map(
                lambda row: row not in self.samples['pos'])
            dataframe = dataframe[dataframe.not_in_pos]

            #dataframe.dropna(subset=['embedding_vector'], inplace=True)

        embeddings = []

        def _safe_get_from_dict(data, path):
            try:
                for node in path:
                    data = data[node]
                return data
            except:
                return None

        if 'embeddings' in dataframe:
            if self.config.embeddings_src == "dssm":
                dataframe['embedding_vector'] = dataframe['embeddings'].map(
                    lambda row: _safe_get_from_dict(json.loads(row), ['Embeddings', 'Dssm', 'Data']))
                dataframe.dropna(subset=['embedding_vector'], inplace=True)
                embeddings = dataframe['embedding_vector'].to_list()
                for i in range(len(embeddings)):
                    embeddings[i] = Dataset._str_to_embedding(embeddings[i])
            elif self.config.embeddings_src == "bert":
                embeddings = (dataframe['embeddings'].map(lambda row: json.loads(row))).to_list()
            else:
                raise ValueError('Expected dssm or bert embeddings, found embedding={}'.format(
                    self.config.embeddings_src))

            samples = dataframe['text'].tolist()
        else:
            if self.config.embeddings_path is None:
                raise ValueError('There is no embeddings in file {}, but base file does not exist.'.format(filename))

            samples = dataframe['text'].tolist()

            if samples is None:
                raise ValueError(
                    'Found neither "text" column with samples'
                    ' nor "embeddings" column with embeddings in file {}'.format(filename))

            if not hasattr(self, 'embeddings_pool'):
                self._get_embeddings_from_storage()

            for sample in samples:
                embeddings.append(self.embeddings_pool[sample])

        embeddings = np.array(embeddings)

        weights = None

        if 'weight' in dataframe:
            weights = dataframe['weight'].to_numpy()
        else:
            weights = np.ones(embeddings.shape[0])

        if need_shuffle:
            indexes = np.arange(len(samples))
            np.random.shuffle(indexes)
            embeddings = embeddings[indexes]
            weights = weights[indexes]
            samples = [samples[idx] for idx in indexes]

        return embeddings, samples, weights

    def _get_embeddings_from_storage(self):
        if self.config.embeddings_path is None:
            raise ValueError('Expected some path to file with embeddings.')

        before_preparing = time.time()

        data = {}
        presaved_path = Dataset._directory_with_presaved_embeddings() + Dataset._convert_path_to_number(self.config.embeddings_path) + '.npy'

        # os.path.exists(presaved_path) and os.path.getmtime(presaved_path) > os.path.getmtime(self.config.embeddings_path)
        if os.path.exists(presaved_path):
            data = np.load(presaved_path, allow_pickle=True).item()
            after_preparing = time.time()
            logging.info('Spend {} seconds on getting {} pre-compiled embeddings for file {}, stored at {}.'.format(
                after_preparing - before_preparing, len(data), self.config.embeddings_path, presaved_path))
        else:
            def sample_from_line(line):
                embedding = json.loads(line.split('\t')[2])['Embeddings']['Dssm']['Data']
                data[line.split('\t')[0]] = Dataset._str_to_embedding(embedding)

            with open(self.config.embeddings_path, 'r') as storage_file:
                next(storage_file)
                for line in storage_file:
                    sample_from_line(line)
            after_preparing = time.time()

            logging.info('Spend {} seconds on parsing file with embeddings {}.'.format(after_preparing - before_preparing, self.config.embeddings_path))

            open(presaved_path, 'w').close()  # file must exist for numpy.save
            np.save(presaved_path, data)
            logging.info('Saved pre-compiled embeddings in file {}.'.format(presaved_path))

        self.embeddings_pool = data

    @staticmethod
    def _convert_path_to_number(s):
        return str(hash(s))

    @staticmethod
    def _directory_with_presaved_embeddings():
        # ARCADIA = os.getenv('ARCADIA') # As I understood, ya make considers $ARCADIA as root.
        # maybe should change to more appropriate location.
        return 'internal_data/'

    @staticmethod
    def _str_to_embedding(s):
        return np.frombuffer(base64.b64decode(s), dtype=np.float16)
