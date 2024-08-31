import argparse
import codecs
import pickle

import numpy as np
from sklearn.feature_extraction.text import CountVectorizer

parser = argparse.ArgumentParser()
parser.add_argument('--input')
parser.add_argument('--output')
args = parser.parse_args()
data = np.loadtxt(codecs.open(args.input, 'rb'), delimiter='\t', dtype=bytes)
cv = CountVectorizer(max_df=0.4, min_df=100)
counts = cv.fit_transform(data)
id2word = {i: w for (w, i) in cv.vocabulary_.items()}
word2id = {w: i for (w, i) in cv.vocabulary_.items()}
data = {
    'counts': counts,
    'id2word': id2word,
    'word2id': word2id
}
pickle.dump(data, open(args.output, 'wb'), protocol=2)
