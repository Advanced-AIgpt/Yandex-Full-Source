import argparse
import os
import pickle

import gensim

parser = argparse.ArgumentParser()
parser.add_argument('--mode', default='train,visualize')
parser.add_argument('--host', default='0.0.0.0')
parser.add_argument('--port', default='8080')
parser.add_argument('--model-name', default='model.dat')
parser.add_argument('--dataset', default='sessions.dat')
args = parser.parse_args()
args.mode = args.mode.split(',')

data = pickle.load(open(args.dataset, 'rb'))
counts = data['counts']
id2word = data['id2word']
word2id = data['word2id']
full_corpus = gensim.matutils.Scipy2Corpus(counts)

if 'train' in args.mode:
    num_topics = 30
    alpha = [0.02] * num_topics
    iterations = 200
    num_passes = 20

    lda = gensim.models.LdaModel(
        corpus=full_corpus,
        passes=num_passes,
        num_topics=num_topics,
        eta='auto',
        iterations=iterations,
        id2word=id2word,
        eval_every=0,
        random_state=42,
        )
    print('ELBO = {0:.4f}'.format(lda.bound(full_corpus)))
    lda.save(args.model_name)
    os.system('ya notify')

if 'visualize' in args.mode:
    class MyDictionary():
        def __init__(self, word2id):
            self.token2id = word2id

        def __len__(self):
            return len(self.token2id)

    class MyScipy2Corpus(gensim.matutils.Scipy2Corpus):
        def __len__(self):
            return self.vecs.shape[0]

    import pyLDAvis.gensim

    lda = gensim.models.LdaModel.load(args.model_name)
    my_full_corpus = MyScipy2Corpus(counts)
    my_dictionary = MyDictionary(word2id)
    data = pyLDAvis.gensim.prepare(lda, my_full_corpus, my_dictionary, sort_topics=False)
    pyLDAvis.show(data, ip=args.host, port=args.port)
