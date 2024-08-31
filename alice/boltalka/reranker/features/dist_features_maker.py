import numpy as np
import regex as re
import codecs, argparse, sys


class Featurizer(object):
    def __init__(self):
        pass

    def convert_str(self, line):
        return np.array([float(x) for x in line.strip().split(' ')], dtype=np.float32)

    def __call__(self, line):
        line = line.rstrip('\n')
        reprs = np.array([self.convert_str(repr_str) for repr_str in line.split('\t')])

        D = np.dot(reprs, reprs.T)

        features = []
        for i in range(len(reprs)):
            for j in range(i+1, len(reprs)):
                features.append(D[i,j])
        return features



def main(embeddings_file):
    featurizer = Featurizer()

    with codecs.open(embeddings_file, 'r', 'utf-8') as f_i:
        for line_idx, line in enumerate(f_i):
            line = line.rstrip('\n')
            features = featurizer(line)
            print '\t'.join([str(x) for x in features])
            if line_idx % 10000 == 0:
                print >> sys.stderr, '%d\r' % line_idx,


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--input', required=True)
    args = parser.parse_args()
    main(args.input)

