#!/usr/bin/env python
# encoding: utf-8
import json


CHECK_PATH = 'tmp/results/check.json'
ASSIGN_PATH = 'tmp/assignments_2.json'


def get_approvements(assign_id):
    with open(CHECK_PATH) as inp:
        for assign in json.load(inp):
            if assign['assignmentId'] == assign_id:
                return assign['status']['value']


def get_overlapped(assign_id):
    with open(ASSIGN_PATH) as inp:
        all_tasks = json.load(inp)

    overlapped = {t['taskId']: {'own': t['outputValues'],
                                'others': []}
                  for t in all_tasks
                  if t['assignmentId'] == assign_id}

    for task in all_tasks:
        if task['taskId'] in overlapped and task['assignmentId'] != assign_id:
            overlapped[task['taskId']]['others'].append(task['outputValues'])

    return overlapped


if __name__ == '__main__':
    ASSIGNMENT_ID = '00078dc0-e230-43a2-bf7f-c7183b318bc9'
    print get_approvements(ASSIGNMENT_ID)

    overlapped = get_overlapped(ASSIGNMENT_ID)

    for task_id, assigns in overlapped.iteritems():
        print '==== %s ====' % task_id
        print u'{speech} {result}'.format(**assigns['own'])
        print '---'
        for a in assigns['others']:
            print u'{speech} {result}'.format(**a)
