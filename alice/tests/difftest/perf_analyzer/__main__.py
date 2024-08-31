import argparse
import json
import math
import matplotlib.pyplot as plt
import os
import random


def setup():
    # set up figure size
    fig_size = plt.rcParams["figure.figsize"]

    fig_size[0] = 12 * 2
    fig_size[1] = 12 * 2
    plt.rcParams["figure.figsize"] = fig_size


def num(values):
    res = []
    pos = 0
    for v in values:
        res.append((pos, v))
        pos += 1
    return res


def calc_diff_stats(times_old, times_new, threshold):
    thresh_diffs = []
    for i in range(len(times_old)):
        new, old = times_new[i], times_old[i]

        if max(new, old):
            diff = new - old
            if abs(diff / max(new, old)) >= threshold:
                thresh_diffs.append(diff)

    if thresh_diffs:
        avg = sum(thresh_diffs) / len(thresh_diffs)
        return (len(thresh_diffs), avg)
    return (0, None)


def analyze(res):
    times_old = []
    times_new = []
    total_deltas = []
    cnt = 0

    old_nans = 0
    new_nans = 0
    common_nans = 0

    for val in res:
        cnt += 1
        old, new = val

        if math.isnan(old) and math.isnan(new):
            old_nans += 1
            new_nans += 1
            common_nans += 1
        elif math.isnan(old):
            old_nans += 1
        elif math.isnan(new):
            new_nans += 1
        else:
            times_old.append(old)
            times_new.append(new)
            total_deltas.append(new - old)

    analyze_info = {}
    analyze_info['analyzed_diffs'] = cnt
    analyze_info['old_fails'] = old_nans
    analyze_info['new_fails'] = new_nans
    analyze_info['common_fails'] = common_nans

    # draw errors rate
    fig, ax = plt.subplots(nrows=3, ncols=1)
    cnts = []
    avgs = []
    weights = []
    for i in range(100):
        threshold = float(i) / 100
        cnt, avg = calc_diff_stats(times_old, times_new, threshold)

        key = 'relative_error_{}'.format(threshold)
        info = {}
        info['count_of_diffs'] = cnt
        if cnt:
            info['avg_delta'] = avg
        analyze_info[key] = info

        cnts.append(cnt)
        avgs.append(avg)
        weights.append(cnt * avg if cnt else 0)

    ax[0].plot(*zip(*num(cnts)), color='blue')
    ax[0].plot(*zip(*num([0] * len(cnts))), color='black')
    ax[0].set_title('Count of diffs from this threshold')

    ax[1].plot(*zip(*num(avgs)), color='red')
    ax[1].plot(*zip(*num([0] * len(avgs))), color='black')
    ax[1].set_title('Average of diffs from this threshold')

    ax[2].plot(*zip(*num(weights)), color='green')
    ax[2].plot(*zip(*num([0] * len(weights))), color='black')
    ax[2].set_title('Average * Count from this threshold')

    plt.savefig('rate.png')

    # draw histogram
    plt.clf()
    times_old.sort()
    times_new.sort()

    fig, ax = plt.subplots(nrows=3, ncols=1)
    for part in [(0, 70), (1, 350), (2, 700)]:
        ind, bins = part
        ax[ind].hist(times_old, bins, range=(0.0, 7.0), facecolor='blue', alpha=0.5)
        ax[ind].hist(times_new, bins, range=(0.0, 7.0), facecolor='red', alpha=0.5)
        ax[ind].set_title('BLUE - old revision, RED - new revision')
    plt.savefig('hist.png')

    # draw sorted
    plt.clf()
    plt.plot(*zip(*num(times_old)), label='Old revision', color='blue')
    plt.legend()
    plt.plot(*zip(*num(times_new)), label='New revision', color='red')
    plt.legend()
    plt.savefig('sorted.png')

    # print percentils
    perc_file = open('perc.txt', 'w')
    for times in [times_old, times_new]:
        if times == times_new:
            perc_file.write('\n### Percentils for new revision ###\n')
        else:
            perc_file.write('\n### Percentils for old revision ###\n')

        for perc in [0.25, 0.50, 0.75, 0.95, 0.99, 0.999]:
            pos = int(perc * (len(times) - 1))
            perc_file.write('Percentil {}: time {} seconds\n'.format(perc, times[pos]))
    perc_file.close()

    return analyze_info


def gen_perf_file(filename):
    n = 1000
    with open(filename, 'w') as f:
        for i in range(n):
            val = 'nan' if random.randint(0, 10) == 0 else random.uniform(0, 10)
            f.write('req{} {}\n'.format(i, val))


def load_map(perf_file):
    res = {}
    with open(os.path.join(perf_file, "result.txt"), 'r') as pf:
        for line in pf:
            reqid, val = line.split()
            res[reqid] = float(val)
    return res


def combined_responses(old_perf_file, new_perf_file):
    old = load_map(old_perf_file)
    new = load_map(new_perf_file)

    res = []
    for reqid in old:
        old_val = old[reqid]
        new_val = new.get(reqid, float('nan'))
        res.append((old_val, new_val))
    return res


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='I will analyze the performance!')
    parser.add_argument('--gen-perf-file', dest='gen_perf_file')
    parser.add_argument('--old-perf-file', dest='old_perf_file')
    parser.add_argument('--new-perf-file', dest='new_perf_file')
    parser.add_argument('--analyze-file', dest='analyze_file')
    args = parser.parse_args()

    if args.gen_perf_file:
        gen_perf_file(args.gen_perf_file)
    elif args.old_perf_file and args.new_perf_file and args.analyze_file:
        setup()
        with open(args.analyze_file, 'w') as af:
            json.dump(analyze(combined_responses(args.old_perf_file, args.new_perf_file)), af, indent=4)
