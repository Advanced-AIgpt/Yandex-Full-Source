import numpy as np
import codecs, argparse, sys, itertools
from collections import Counter


def read_matrixnet_file(file):
	query_ids, labels, scores = [], [], []
	with codecs.open(file, 'r', 'utf-8') as f:
		for line in f:
			parts = line.rstrip('\n').split('\t')
			query_ids.append(int(parts[0], 16))
			labels.append(float(parts[1]))
			scores.append(float(parts[-1]))
	return np.array(query_ids), np.array(labels), np.array(scores)


# label = 8 * result + 4 * not_rude + 2 * not_male + not_you
def decode_labels(labels):
	relev = labels // 8
	not_male = labels % 4 // 2
	not_rude = labels % 8 // 4
	not_you = labels % 2
	return relev, not_male, not_rude, not_you


def calc_dcg(relev):
	return np.sum((np.exp2(relev) - 1) / np.log2(np.arange(len(relev)) + 2))


def calc_ndcg(relev):
	dcg = calc_dcg(relev)
	idcg = calc_dcg(-np.sort(-relev))
	if idcg == 0.:
		assert dcg == 0.
		return 1.
	return dcg / idcg


def calc_metrics(labels, scores, ks):
	#assert len(labels) >= max(ks):
	order = np.argsort(-scores)
	relev_ordered, not_male_ordered, not_rude_ordered, not_you_ordered = labels[order].T

	metrics = {}
	for k in ks:
		metrics['bad@%d' % k] = (relev_ordered[:k] == 0).mean()
		metrics['neutral@%d' % k] = (relev_ordered[:k] == 1).mean()
		metrics['good@%d' % k] = (relev_ordered[:k] == 2).mean()
		metrics['score@%d' % k] = relev_ordered[:k].mean()
		metrics['male@%d' % k] = 1. - not_male_ordered[:k].mean()
		metrics['rude@%d' % k] = 1. - not_rude_ordered[:k].mean()
		metrics['you@%d' % k] = 1. - not_you_ordered[:k].mean()
	metrics['relev_ndcg'] = calc_ndcg(relev_ordered)
	metrics['male_ndcg'] = calc_ndcg(not_male_ordered)
	metrics['rude_ndcg'] = calc_ndcg(not_rude_ordered)
	metrics['you_ndcg'] = calc_ndcg(not_you_ordered)
	return metrics


def get_test_loss(log_file):
    test_loss = []
    is_matrixnet = True
    with codecs.open(log_file, 'r', 'utf-8') as f:
    	first_line = f.readline()
    	if first_line.split('\t')[0] == 'iter':
    		is_matrixnet = False
    	else:
    		f = itertools.chain([first_line], f)
    	test_loss = np.array([float(line.rstrip('\n').split('\t')[1]) for line in f])
    if is_matrixnet:
    	test_loss *= -1.0
    iteration = np.argmin(test_loss)
    return (test_loss[iteration], iteration)


def main(args):
	query_ids, encoded_labels, scores = read_matrixnet_file(args.eval)
	relev, not_male, not_rude, not_you = decode_labels(encoded_labels)
	labels = np.vstack([relev, not_male, not_rude, not_you]).T

	if args.mode == 'baseline':
		scores = reversed(range(len(labels)))
	elif args.mode == 'best':
		scores = eval(args.target)
	elif args.mode == 'worst':
		scores = -eval(args.target)
	else:
		assert args.mode is None

	ks = [int(x) for x in args.k.split(',')]

	metrics = Counter()
	prev_query_id = None
	num_groups = 0
	num_samples = len(labels)

	local_labels = []
	local_scores = []

	for sample_id, (query_id, label, score) in enumerate(zip(query_ids, labels, scores)):
		if (sample_id != 0 and query_id != prev_query_id) or sample_id == num_samples - 1:
			local_labels = np.array(local_labels)
			local_scores = np.array(local_scores)
			metrics.update(calc_metrics(local_labels, local_scores, ks))
			local_labels = []
			local_scores = []
			num_groups += 1
		local_labels.append(label)
		local_scores.append(score)
		prev_query_id = query_id

	num_groups = float(num_groups)

	for name in metrics:
		metrics[name] /= num_groups
		if name.startswith(('bad@', 'neutral@', 'good@', 'male@', 'rude@', 'you@')):
			metrics[name] *= 100

	metric_names = []
	for k in ks:
		metric_names.extend([name+str(k) for name in ['bad@', 'neutral@', 'good@', 'score@', 'male@', 'rude@', 'you@']])
	metric_names.extend(['relev_ndcg', 'male_ndcg', 'rude_ndcg', 'you_ndcg'])


	if args.log:
		test_loss, iteration = get_test_loss(args.log)
		name = 'log-loss (iter)'
		metrics[name] = (test_loss, iteration)
		metric_names.insert(0, name)


	def print_score(dct, name):
		if name == 'log-loss (iter)':
			test_loss, iteration = dct[name]
			print '%.4f (%d)' % (test_loss, iteration),
		elif name.endswith('ndcg'):
			print '%.5f' % dct[name],
		else:
			print '%.3f' % dct[name],


	if args.oneliner:
		print '|| %s' % metric_names[0],
		for name in metric_names[1:]:
			print '| %s' % name,
		print '||'
		print '||',
		print_score(metrics, metric_names[0])
		for name in metric_names[1:]:
			print '|',
			print_score(metrics, name)
		print '||'
	else:
		for name in metric_names:
			print name + ' =',
			print_score(metrics, name)
			print '\n',


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--eval', required=True)
    parser.add_argument('--log', default=None)
    parser.add_argument('--mode', choices=['baseline', 'best', 'worst'], default=None)
    parser.add_argument('--target', choices=['relev', 'not_male', 'not_rude', 'not_you'], default='relev')
    parser.add_argument('--oneliner', action='store_true')
    parser.add_argument('-k', type=str, default='1')
    args = parser.parse_args()
    main(args)
