import gensim
import yt.wrapper as yt
import pickle
import numpy as np
import argparse
from collections import Counter


class TopicSuggester:
    def __init__(self, base_path):
        self.lda = gensim.models.ldamodel.LdaModel.load(base_path + "model.dat")
        dictionary_data = pickle.load(open(base_path + 'sessions.dat', "rb"))
        self.word2id = dictionary_data['word2id']
        self.per_topic = np.load(base_path + 'per_topic.npy')
        self.phrases = pickle.load(open(base_path + 'id2phrase.pkl', 'rb'))

    def __call__(self, row):
        pred = self.lda.get_document_topics(self.bow(row['session']))
        topics = np.zeros((1, self.per_topic.shape[0]))
        for el in pred:
            topics[0][el[0]] = el[1]
        topics /= np.sum(topics, axis=1, keepdims=True)
        per_doc = np.dot(topics, self.per_topic)
        yield dict(uuid=row['uuid'], topics=np.random.choice(self.phrases, 10, replace=False, p=per_doc[0]))

    def bow(self, doc):
        doc = doc.split()
        c = Counter([self.word2id.get(el) for el in doc])
        del c[None]
        s = sum(c[k] for k in c)
        res = [(k, c[k] * 1.0 / s) for k in c]
        return res

if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--model', required=True)
    args = parser.parse_args()
    yt.config["proxy"]["url"] = "hahn"
    yt.run_map(TopicSuggester(args.model), args.src, args.dst)
