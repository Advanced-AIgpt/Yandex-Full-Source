import numpy as np
import codecs, argparse, sys
from copy import copy
from collections import Counter


def read_matrixnet_file(file):
	query_ids, labels, scores = [], [], []
	with codecs.open(file, 'r', 'utf-8') as f:
		for line in f:
			parts = line.rstrip('\n').split('\t')
			query_ids.append(int(parts[0]))
			labels.append(float(parts[1]))
			scores.append(float(parts[-1]))
	return np.array(query_ids), np.array(labels), np.array(scores)


def read_floats(file):
	vals = []
	with codecs.open(file, 'r', 'utf-8') as f:
		for line in f:
			vals.append(float(line.rstrip('\n')))
	return np.array(vals)


def calc_metrics(labels, scores, ks):
	#assert len(labels) >= max(ks):
	order = np.argsort(-scores)
	labels_ordered = labels[order]
	top_size = len(labels)
	total = {k: labels_ordered[:k].sum() / float(min(k, top_size)) for k in ks}
	bad = {k: (labels_ordered[:k] == 0).sum() / float(min(k, top_size)) for k in ks}
	neutral = {k: (labels_ordered[:k] == 1).sum() / float(min(k, top_size)) for k in ks}
	good = {k: (labels_ordered[:k] == 2).sum() / float(min(k, top_size)) for k in ks}
	return total, bad, neutral, good


def main(args):
	query_ids, labels, scores = read_matrixnet_file(args.eval)
	if args.labels:
		labels = read_floats(args.labels)
	if args.baseline:
		scores = reversed(range(len(labels)))
	elif args.best:
		scores = labels
	elif args.worst:
		scores = -labels

	ks = [int(x) for x in args.k.split(',')]

	majority_total = Counter()
	majority_bad = Counter()
	majority_neutral = Counter()
	majority_good = Counter()

	prev_query_id = -1
	num_groups = 0

	local_labels = []
	local_scores = []

	for query_id, label, score in zip(query_ids, labels, scores):
		if prev_query_id != -1 and query_id != prev_query_id:
			local_labels = np.array(local_labels)
			local_scores = np.array(local_scores)
			total, bad, neutral, good = calc_metrics(local_labels, local_scores, ks)
			majority_total.update(total)
			majority_bad.update(bad)
			majority_neutral.update(neutral)
			majority_good.update(good)
			local_labels = []
			local_scores = []
			num_groups += 1
		local_labels.append(label)
		local_scores.append(score)
		prev_query_id = query_id


	local_labels = np.array(local_labels)
	local_scores = np.array(local_scores)
	total, bad, neutral, good = calc_metrics(local_labels, local_scores, ks)
	majority_total.update(total)
	majority_bad.update(bad)
	majority_neutral.update(neutral)
	majority_good.update(good)
	num_groups += 1

	for k in ks:
		majority_total[k] /= num_groups
		majority_bad[k] = 100*majority_bad[k]/num_groups
		majority_neutral[k] = 100*majority_neutral[k]/num_groups
		majority_good[k] = 100*majority_good[k]/num_groups

	if args.oneliner:
		for k in ks:
			print '| %.2f | %.2f | %.2f | %.2f' % (majority_bad[k], majority_neutral[k], majority_good[k], majority_total[k]),
		print '||'
	else:
		for k in ks:
			print 'Total@%d\t\t%.3f' % (k, majority_total[k])
			print '0@%d\t\t%.3f' % (k, majority_bad[k])
			print '1@%d\t\t%.3f' % (k, majority_neutral[k])
			print '2@%d\t\t%.3f' % (k, majority_good[k])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--eval', required=True)
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--best', action='store_true')
    parser.add_argument('--worst', action='store_true')
    parser.add_argument('-k', type=str, default='1,5')
    parser.add_argument('--oneliner', action='store_true')
    parser.add_argument('--labels', default=None)
    args = parser.parse_args()
    assert args.baseline + args.best + args.worst < 2
    main(args)
