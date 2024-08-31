import logging
import time
from typing import List, Dict

from sandbox import common
from sandbox.projects.release_machine import client

from alice.tasklet.check_rm_tests.proto import check_rm_tests_tasklet

TERMINAL_TASK_STATES = (
    'TASK_SUCCESS',
    'TASK_FAILURE',
    'TASK_EXCEPTION'
)


class CheckRmTestsImpl(check_rm_tests_tasklet.CheckRmTestsBase):
    def run(self):
        self.rm_client = client.RMClient()
        self.output_data = dict()

        self.retries_config = {
            'sleep_time': self.input.config.sleep_time,
            'retries': self.input.config.retries,
            'backoff': self.input.config.backoff
        }

        self._wait_for_data()
        for k in self.output_data:
            cur = self.output_data[k]
            self.output.task_states.resources[k] = cur['status']
        if not all([x == 'TASK_SUCCESS' for x in self.output.task_states.resources.values()]):
            raise common.errors.TaskError('Not all RM tests are green')
        self.output.state.success = True

    def _wait_for_data(self):
        self._wait_for(self._check_for_all_tasks_done_or_dead)

    def _check_for_all_tasks_done_or_dead(self):
        events = self._get_events_for_revision()
        self._report_readiness(events)
        for cur in self.output_data.values():
            if cur['status'] not in TERMINAL_TASK_STATES:
                return False

    def _get_events_for_revision(self) -> List:
        events = self.rm_client.get_events(self.input.config.component_name, "GenericTest")
        return [x for x in events['events'] if self._is_relevant_event(x['event']['genericTestData'])]

    def _report_readiness(self, events: List):
        for event in events:
            id = event['event']['genericTestData']['jobName']
            if id not in self.output_data.keys():
                self.output_data[id] = {
                    'status': event['taskStatus'],
                    'timestamp': event['timestamp']
                }
                continue
            if event['timestamp'] > self.output_data[id]['timestamp']:
                self.output_data[id]['status'] = event['taskStatus'],
                self.output_data[id]['timestamp'] = event['timestamp']

    def _wait_for(self, fun, *args):
        retry_counter = 0
        sleep_for = self.retries_config['sleep_time']
        while retry_counter < self.retries_config['retries']:
            result = fun(*args)
            if result:
                return result
            logging.info('Data is not ready yet. Sleeping for %d seconds (retry %d)', sleep_for, retry_counter)
            time.sleep(sleep_for)
            sleep_for = sleep_for * self.retries_config['backoff']
            retry_counter += 1

    def _is_relevant_event(self, event: Dict) -> bool:
        return all([
            event.get('revision', None) == str(self.input.config.base_commit_id),
            event.get('scopeNumber', None) == str(self.input.config.branch)
        ])
