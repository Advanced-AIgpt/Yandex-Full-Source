import argparse
import codecs
from collections import Counter
import pickle

import gensim
import numpy as np


def bow(doc, word2id):
    doc = doc.split()
    c = Counter([word2id.get(el.decode('utf-8', errors='ignore')) for el in doc])
    del c[None]
    s = sum(c[k] for k in c)
    res = [(k, c[k] * 1.0 / s) for k in c]
    return res


parser = argparse.ArgumentParser()
parser.add_argument('--dictionary', default='sessions.dat')
parser.add_argument('--dataset', default='session_topic.tsv')
parser.add_argument('--lda-model', default='model.dat')
args = parser.parse_args()

dictionary_data = pickle.load(open(args.dictionary, 'rb'))
lda = gensim.models.LdaModel.load(args.lda_model)
id2word = dictionary_data['id2word']
word2id = dictionary_data['word2id']
data = np.loadtxt(codecs.open(args.dataset, 'rb'), delimiter='\t', dtype=bytes, comments=None)
gt_phrases = [el.decode('utf-8', errors='ignore') for el in data[:, 1]]
pred = [lda.get_document_topics(bow(data[i][0], word2id)) for i in range(len(data))]
id2phrase = pickle.load(open('id2phrase.pkl', 'rb'))
phrase2id = {phrase.lower(): i for i, phrase in enumerate(id2phrase)}
data = [(phrase, vec) for phrase, vec in zip(gt_phrases, pred) if phrase.lower() in phrase2id]
gt_phrases = [el[0] for el in data]
pred = [el[1] for el in data]
topics = np.zeros((len(pred), lda.num_topics))
for i in range(len(pred)):
    for el in pred[i]:
        topics[i][el[0]] = el[1]

labels = np.array([phrase2id[phrase.lower()] for phrase in gt_phrases])

ind = np.array([labels == k for k in range(len(id2phrase))]).T
per_topic = np.random.rand(lda.num_topics, len(id2phrase))
per_topic /= np.sum(per_topic, axis=1, keepdims=True)
for i in range(50):
    per_doc = np.dot(topics, per_topic)
    prev = np.mean([per_doc[i, labels[i]] for i in range(len(per_doc))])
    z = per_topic[:, labels]
    z = z.T * topics
    z /= np.sum(z, axis=1, keepdims=True)
    per_topic = np.sum(z[:, :, None] * ind[:, None, :], axis=0)
    per_topic /= np.sum(per_topic, axis=1, keepdims=True)
    per_doc = np.dot(topics, per_topic)
    cur = np.mean([per_doc[i, labels[i]] for i in range(len(per_doc))])
    print(i, '->', cur, cur - prev)

np.save('per_topic.npy', per_topic)
