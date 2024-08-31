#!/usr/bin/env python
# encoding: utf-8
import json
from nirvana.job_context import context



def main_stat():
    ctx = context()
    inputs = ctx.get_inputs()
    outputs = ctx.get_outputs()

    references = json.load(open(inputs.get('validation')))
    assignments = json.load(open(inputs.get('assignments')))

    stat = {'total': 0,    # Общее количество тасков
            'correct': 0,  # Количество корректных оценок
            'correct_bad': 0,  # Количество корректно оцененых как "bad"
            'correct_not_bad': 0,   # Количество корректно оцененых как "не-bad"
            'accuracy': None,  # Доля тасков размеченных корректно
            'accuracy_bad_cases': None,  # Доля корректных "bad" и "не-bad"
            }

    ref_answers = {ref['key']: ref['true'] for ref in references}

    for task in assignments:
        key = task['inputValues']['key']
        ref_ans = ref_answers.get(key)
        if ref_ans is not None:
            toloka_ans = task['outputValues']['result']

            stat['total'] += 1
            if toloka_ans == 'bad' and ref_ans == 'bad':
                stat['correct_bad'] += 1
            elif toloka_ans != 'bad' and ref_ans != 'bad':
                stat['correct_not_bad'] += 1

            if ref_ans == toloka_ans:
                stat['correct'] += 1

    if stat['total']:
        stat['accuracy'] = float(stat['correct']) / stat['total']
        stat['accuracy_bad_cases'] = float(stat['correct_bad'] + stat['correct_not_bad']) / stat['total']

    json.dump(stat, open(outputs.get('stat'), 'w'), sort_keys=True, indent=2)


if __name__ == '__main__':
    main_stat()

