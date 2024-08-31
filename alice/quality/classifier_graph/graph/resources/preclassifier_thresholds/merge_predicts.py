import collections
import json
import nirvana.job_context as nv
import re


Row = collections.namedtuple('Row', ['id', 'mark', 'text', 'unk', 'score'])
JoinedRow = collections.namedtuple('Row', ['id', 'mark', 'text', 'unk', 'score'])

SCENARIO_SLICES = {
    "music": 'alice_music_scenario',
    "video": 'alice_video_scenario',
}


def main():
    assert u'music' == "music"
    output = ''
    slices = {}
    combined = {}
    features_path = ''
    for input_file in nv.context().get_inputs().get_item_list('infiles'):
        with open(input_file.get_path()) as f:
            if input_file.get_link_name() == 'slices':
                raw_slices = f.read().split()
                for slice in raw_slices:
                    result = re.match(r'(.+)\[(\d+);(\d+)\)', slice)
                    slices[result.group(1)] = (int(result.group(2)), int(result.group(3)))
                output += json.dumps(slices)
            elif input_file.get_link_name() == 'thresholds':
                thresholds = json.load(f)
            elif input_file.get_link_name() == 'features':
                features_path = input_file.get_path()
            else:
                combined[input_file.get_link_name()] = f.readlines()

    marks = collections.defaultdict(dict)
    pre_scores = collections.defaultdict(dict)
    post_scores = collections.defaultdict(dict)
    for key, value in combined.items():
        clf, collection = key.split('_')
        for i, line in enumerate(value):
            row = Row(*line.split('\t'))
            marks[i][collection] = int(row.mark)
            if clf == 'pre':
                pre_scores[i][collection] = float(row.score)
            elif clf == 'post':
                post_scores[i][collection] = float(row.score)
            else:
                raise Exception('Unexpected clf ' + clf)

    results = []
    for id in marks:
        entry = {}
        for collection, value in marks[id].items():
            entry[collection + '_mark'] = value
        entry['pre_predicts'] = pre_scores[id]
        entry['post_predicts'] = post_scores[id]
        results.append(entry)

    result = '\n'.join(json.dumps(entry, ensure_ascii=False) for entry in results)
    with open(nv.context().get_outputs().get('out_text'), 'w') as f:
        f.write(result)


main()
