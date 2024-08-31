import collections
import json
import sys
import copy
import nirvana.job_context as nv

import random

def get_confident_threshold(data, scenario, target_precision):
    scenario_predicts = []
    for parsed in data:
        pre_predicts = parsed['pre_predicts']

        if not pre_predicts:
            continue

        mark = parsed[scenario + "_mark"]
        scenario_predicts.append((pre_predicts[scenario], mark))

    scenario_predicts = sorted(scenario_predicts)

    true_positive = 0.0
    positive = 0.0 
    threshold = scenario_predicts[-1][0] + 1.0
    for predict, mark in reversed(scenario_predicts):
        true_positive += mark
        positive += 1
        if (true_positive / positive) >= target_precision:
            threshold = predict
    return threshold


def get_threshold(data, scenario, target_recall):
    scenario_predicts = []
    trues = 0.0
    for parsed in data:
        pre_predicts = parsed['pre_predicts']

        if not pre_predicts:
            continue

        mark = parsed[scenario + "_mark"]
        scenario_predicts.append((pre_predicts[scenario], mark))
        trues += mark

    scenario_predicts = sorted(scenario_predicts)

    true_positive = trues
    threshold = scenario_predicts[0][0] - 1.0
    for predict, mark in scenario_predicts:
        true_positive -= mark
        if (true_positive / trues) >= target_recall:
            threshold = predict
    return threshold


def test_thresholds(data, thresholds, print_result=False, results=None):
    all_pre_counts = collections.Counter()
    cut_counts = collections.Counter()
    bad_confident_counts = collections.Counter()
    bad_cuts_counts = collections.Counter()
    true_not_cut_post_counts = 0
    have_goods_left = 0
    cut_good_count = 0
    cut_sizes = []

    pos = -1
    post_counts = 0
    true_post_counts = 0
    have_good = 0
    all_goods_cnt = 0
    for parsed in data:
        pos += 1
        pre_predicts = parsed['pre_predicts']
        post_predicts = copy.copy(parsed['post_predicts'])
        if not pre_predicts:
            continue

        if not post_predicts:
            post_predicts = copy.copy(pre_predicts)

        post_scenarios, post_scores = zip(*post_predicts.items())
        _, old_winner = max(zip(post_scores, post_scenarios))

        true_post_counts += parsed[old_winner + '_mark']

        found_good = False
        for scenario in post_predicts:
            mark = parsed[scenario + '_mark']
            found_good |= mark
            all_goods_cnt += mark

        have_good += found_good
        post_counts += 1

        potential_cuts = []
        found_confident = parsed['forced_confident'] if 'forced_confident' in parsed else False
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

        if potential_cuts:
            _, cut_scenarios = zip(*potential_cuts)
        else:
            cut_scenarios = []

        had_good_answer = False
        for scenario in cut_scenarios:
            if parsed[scenario + '_mark'] == 1:
                cut_good_count += 1
                had_good_answer = True
                bad_cuts_counts[scenario] += 1
            del post_predicts[scenario]

        if len(post_predicts) > 0:
            if len(post_predicts) > 1:
                post_scenarios, post_scores = zip(*post_predicts.items())
                _, winner = max(zip(post_scores, post_scenarios))
            else:
                winner = next(iter(post_predicts.keys()))

            if parsed[winner + '_mark'] == 1:
                true_not_cut_post_counts += 1

        has_good_left = False
        for scenario in post_predicts:
            if parsed[scenario + '_mark'] == 1:
                has_good_left = True
                break

        if not has_good_left and had_good_answer:
            for scenario, score in pre_predicts.items():
                if score > thresholds['confident_' + scenario]:
                    bad_confident_counts[scenario] += 1

        if has_good_left:
            have_goods_left += 1
        elif results is not None and had_good_answer == True:
            print("{} Cut all goods!".format(results[pos]))
        #if parsed[old_winner + '_mark'] == 0 and parsed[winner + '_mark'] == 1 and results is not None:
        #    print("{} post_winner: {} updated_to: {}".format(results[pos][2], old_winner, winner))

    all_pre_counts_total = 0
    cut_counts_total = 0
    for scenario in cut_counts:
        all_pre_counts_total += all_pre_counts[scenario]
        cut_counts_total += cut_counts[scenario]
        
    base_quality = 100.0 * true_post_counts / post_counts
    after_cut = 100.0 * true_not_cut_post_counts / post_counts
    cut_goods = 100.0 * cut_good_count / all_goods_cnt
    have_goods_left_part = 100.0 * have_goods_left / have_good
    cut_total = 100.0 * cut_counts_total / all_pre_counts_total

    if print_result:
        with open(nv.context().get_outputs().get('out_text'), 'w') as out_f:
            for scenario in cut_counts:
                out_f.write("{}: total {}, cut {} ({}%)\n".format(scenario, all_pre_counts[scenario], cut_counts[scenario], 100.0 * cut_counts[scenario] / all_pre_counts[scenario]))
            out_f.write("---------------------------\n")
            out_f.write("Total all {}, total cut {} ({}%)\n".format(all_pre_counts_total, cut_counts_total, cut_total))
            out_f.write("---------------------------")
            
            out_f.write("Base quality: {}%\n".format(base_quality))
            out_f.write("Quality after cut: {}%\n".format(after_cut))
            out_f.write("Cut good answers: {}%\n".format(cut_goods))
            out_f.write("Have good answer left: {}%\n".format(have_goods_left_part))
            out_f.write("Quality drop: {}%\n".format(base_quality - after_cut))
            out_f.write("---------------------------\n")
            out_f.write("Good scenario was cut:\n")
            out_f.write(str(bad_cuts_counts))
            out_f.write("\nScenario was false confident:\n")
            out_f.write(str(bad_confident_counts))
            out_f.write("\n---------------------------\n")
            out_f.write("exp: mm_preclassifier_thresholds={}\n".format(";".join("{}:{}".format(s, str(t)) for (s, t) in thresholds.items())))
        with open(nv.context().get_outputs().get('out_json'), 'w') as jfile:
            json.dump(thresholds, jfile)


def main():
    grid = {
        "search_recall": 0.95,
        "search_precision": 0.95,
        "vins_precision": 0.95,
        "video_precision": 0.95,
        "music_precision": 0.95,
        "video_recall": 0.95,
        "music_recall": 0.98,
        "vins_recall": 0.98
    }

    texts = []
    data = []
    for input_file in nv.context().get_inputs().get_item_list('infiles'):
        with open(input_file.get_path()) as f:
            if input_file.get_link_name() == 'grid':
                grid = json.load(f)
            elif input_file.get_link_name() == 'texts':
                for line in f:
                    texts.append(line.strip())
            elif input_file.get_link_name() == 'thresholds':
                thresholds = json.load(f)
            elif input_file.get_link_name() != 'forced_confident':
                for line in f:
                    parsed = json.loads(line)
                    data.append(parsed)

    for input_file in nv.context().get_inputs().get_item_list('infiles'):
        with open(input_file.get_path()) as f:
            if input_file.get_link_name() == 'forced_confident':
                for i, line in enumerate(f):
                    forced = line.strip() == 'true'
                    data[i]['forced_confident'] = forced

    if 'input-params' in nv.context().get_parameters():
        tune_mode = nv.context().get_parameters()['input-params']
    else:
        tune_mode = ''

    if tune_mode != 'force':
        for sc in grid:
            scenario = sc.split('_')[0]
            if '_precision' in sc:
                thresholds['confident_' + scenario] = get_confident_threshold(data, scenario, grid[sc])
            else:
                thresholds[scenario] = get_threshold(data, scenario, grid[sc])

    test_thresholds(data, thresholds, print_result=True, results=texts)
    print(thresholds)


main()
