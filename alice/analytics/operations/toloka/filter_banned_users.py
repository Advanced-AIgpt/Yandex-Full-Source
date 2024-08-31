#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation
from utils.toloka.api import ProjectRequester


def filter_banned_users(assignments, is_sandbox, prj_id):
    project_requester = ProjectRequester(is_sandbox=is_sandbox, prj_id=prj_id)
    banned_workers = set(user['user_id'] for user in project_requester.list_banned_users()['items'])

    worker_to_last_task_suite = get_worker_to_last_task_suite_mapping(assignments)

    filtered_assignments = []
    for assignment in assignments:
        is_last_task_suite = worker_to_last_task_suite[assignment['workerId']] == assignment['taskSuiteId']

        # we filter out the assignment if a worker is banned and the assignment is within the last task suite
        if assignment['workerId'] in banned_workers and is_last_task_suite:
            continue

        filtered_assignments.append(assignment)
    return filtered_assignments


def get_worker_to_last_task_suite_mapping(assignments):
    worker_to_last_assignment = dict()
    for assignment in assignments:
        worker_id = assignment['workerId']
        if worker_id not in worker_to_last_assignment or \
                worker_to_last_assignment[worker_id]['submitTs'] < assignment['submitTs']:
            worker_to_last_assignment[worker_id] = assignment
    worker_to_last_task_suite = {
        worker_id: assignment['taskSuiteId']
        for worker_id, assignment in worker_to_last_assignment.iteritems()
    }

    return worker_to_last_task_suite


if __name__ == '__main__':
    call_as_operation(filter_banned_users, input_spec={
        "assignments": {"parser": "json"},
    })
