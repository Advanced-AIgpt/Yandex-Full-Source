import collections
import json
import nirvana.job_context as nv


def main():
    all_pre_counts = collections.Counter()
    cut_counts = collections.Counter()

    post_counts = 0
    true_post_counts = 0
    true_not_cut_post_counts = 0
    cut_sizes = []
    data = []
    for input_file in nv.context().get_inputs().get_item_list('infiles'):
        with open(input_file.get_path()) as f:
            if input_file.get_link_name() == 'thresholds':
                thresholds = json.load(f)
            else:
                for line in f:
                    parsed = json.loads(line)
                    data.append(parsed)

    for parsed in data:
        pre_predicts = parsed['pre_predicts']
        post_predicts = parsed['post_predicts']
        if not pre_predicts or not post_predicts:
            continue

        potential_cuts = []
        found_confident = False
        for scenario, score in pre_predicts.items():
            if score < thresholds.get(scenario, score):
                potential_cuts.append((score, scenario))
            if parsed[scenario + "_mark"] < 1:
                all_pre_counts[scenario] += 1
            if score > thresholds['confident_' + scenario]:
                found_confident = True
        if not found_confident:
            potential_cuts = []

        cut_sizes.append(len(potential_cuts))
        for _, scenario in potential_cuts:
            cut_counts[scenario] += 1

        post_scenarios, post_scores = zip(*post_predicts.items())
        _, winner = max(zip(post_scores, post_scenarios))

        if parsed[winner + '_mark'] == 1:
            true_post_counts += 1
        post_counts += 1

        if potential_cuts:
            _, cut_scenarios = zip(*potential_cuts)
        else:
            cut_scenarios = []

        for scenario in cut_scenarios:
            del post_predicts[scenario]
        if len(post_predicts) > 1:
            post_scenarios, post_scores = zip(*post_predicts.items())
            _, winner = max(zip(post_scores, post_scenarios))
        else:
            winner = next(iter(post_predicts.keys()))
        if parsed[winner + '_mark'] == 1:
            true_not_cut_post_counts += 1

    with open(nv.context().get_outputs().get('out_text'), 'w') as out_f:
        all_pre_counts_total = 0
        cut_counts_total = 0
        for scenario in cut_counts:
            all_pre_counts_total += all_pre_counts[scenario]
            cut_counts_total += cut_counts[scenario]
            out_f.write("{}: total {}, cut {} ({}%)\n".format(scenario, all_pre_counts[scenario], cut_counts[scenario], 100.0 * cut_counts[scenario] / all_pre_counts[scenario]))
        out_f.write("---------------------------\n")
        out_f.write("Total all {}, total cut {} ({}%)\n".format(all_pre_counts_total, cut_counts_total, 100.0 * cut_counts_total / all_pre_counts_total))
        out_f.write("---------------------------")
        base_quality = 100.0 * true_post_counts / post_counts
        after_cut = 100.0 * true_not_cut_post_counts / post_counts
        out_f.write("Base quality: {}%\n".format(base_quality))
        out_f.write("Quality after cut: {}%\n".format(after_cut))
        out_f.write("Quality drop: {}%\n".format(base_quality - after_cut))
        out_f.write("---------------------------\n")
        out_f.write("exp: mm_preclassifier_thresholds={}\n".format(";".join("{}:{}".format(s, str(t)) for (s, t) in thresholds.items())))
    with open(nv.context().get_outputs().get('out_json'), 'w') as jfile:
        json.dump({'total_cuts': 1.0 * cut_counts_total / all_pre_counts_total}, jfile)


main()
