# coding=utf-8
import itertools
import sys
import argparse
import codecs


def main(args):
    factor_names = ["static_{}".format(i) for i in range(9)]
    query = ["query_{}".format(i) for i in reversed(range(3))]
    context = ["context_{}".format(i) for i in reversed(range(3))]
    reply = ["reply"]
    turn_names = query + context + reply
    factor_names += ["dssm_context_reply", "dssm_query_context", "dssm_query_reply", "dssm_weighted"]
    factor_names += list(itertools.product(turn_names, ["punct_{}".format(el) for el in "?!.,()"] + ["num_words", "sum_word_lens", "mean_word_len", "min_word_len", "max_word_len"]))
    full_query = query + ["full_query"]
    full_context = context + ["full_context", "reply"]
    factor_names += list(itertools.product(["word_intersection", "3gram_intersection"],
        list(itertools.combinations(full_query, 2)) +
        list(itertools.combinations(full_context, 2)) +
        list(itertools.product(full_query, full_context))
    ))
    factor_names += list(itertools.product(turn_names, [u"я", u"что", u"ты", u"нет", u"да", u"кто", u"вы", u"он", u"она", u"сама", u"сам", u"они", u"сами", u"само"]))
    factor_names += list(itertools.product(turn_names, ["tag_{}".format(i) for i in range(39)] + ["oov"]))
    factor_names += ["factor_dssm_context_reply", "factor_dssm_query_context", "factor_dssm_query_reply", "factor_dssm_weighted"]
    factor_names += ["is_factor_dssm", "informativeness", "is_s2s", "s2s_score"]

    if not args.fstr:
        for factor_id, name in enumerate(factor_names):
            print factor_id, '\t', name
        print("Total {} factors".format(len(factor_names)))
    else:
        factor_ids = []
        max_factor_id = 0
        with codecs.open(args.fstr) as f:
            for line in f:
                score, factor_id = line.split()
                factor_id = int(factor_id)
                factor_ids.append(factor_id)
                print score, '\t', factor_id, '\t', factor_names[factor_id]
                max_factor_id = max(factor_id, max_factor_id)
        print("Total {} factors. Found {} in file".format(len(factor_names), max_factor_id + 1))
        num_ignored = int(len(factor_names) * args.ignore_fraction)
        if num_ignored:
            print(':'.join(map(str, factor_ids[:-num_ignored-1:-1])))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--fstr', default=None)
    parser.add_argument('--ignore-fraction', type=float, default=0.)
    args = parser.parse_args()
    main(args)
