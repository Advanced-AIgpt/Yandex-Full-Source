import logging
import time
from typing import Optional

from ci.tasklet.common.proto import service_pb2 as ci

from alice.tasklet.create_new_rm_branch.proto import create_new_rm_branch_tasklet
from sandbox import sdk2, common
from sandbox.projects.release_machine import client
from sandbox.projects.release_machine.components import all as all_rm_components

RM_URL_TEMPLATE = 'https://rm.z.yandex-team.ru/component/{}/manage?detail=job_results&scopes=3&branch={}&tag={}'


class CreateNewRmBranchImpl(create_new_rm_branch_tasklet.CreateNewRmBranchBase):
    def run(self):
        self.rm_client = client.RMClient()
        self.retries_config = {
            'sleep_time': self.input.config.sleep_time,
            'retries': self.input.config.retries,
            'backoff': self.input.config.backoff
        }

        self.working_rm_info = dict()

        # major_version = int(self.input.config.major_version)
        minor_version = int(self.input.config.minor_version)
        resource_names = self.input.config.resource_name

        c_info = all_rm_components.get_component(self.input.config.component_name)

        revision = self.input.config.svn_revision

        branch_no = int(c_info.last_scope_num)

        if minor_version == 0 and self.input.context.job_instance_id.number == 1:
            branch_no += 1
            self._spawn_new_branch(revision, c_info, branch_no)

        self.working_rm_info['revision'] = self._wait_for(self._get_first_commit_id, self.input.config.component_name,
                                                          branch_no)
        progress_report = self._prepare_progress_update(branch_no, self.working_rm_info['tag'])
        self.ctx.ci.UpdateProgress(progress_report)

        resources = self._get_resources(branch_no, self.working_rm_info['base_commit_id'],
                                        resource_names)

        for k, v in resources.items():
            self.output.resources.resources[k] = v
        self.output.state.branch = branch_no
        self.output.state.tag = int(self.working_rm_info['tag'])
        self.output.state.first_tag_revision = int(self.working_rm_info['revision'])
        self.output.state.base_commit_id = int(self.working_rm_info['base_commit_id'])
        self.output.state.success = True

    def _prepare_progress_update(self, branch, tag):
        rm_url = RM_URL_TEMPLATE.format(self.input.config.component_name, branch, tag)

        progress = ci.TaskletProgress()
        progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress.id = 'RM url'
        progress.text = rm_url
        progress.url = rm_url
        progress.module = "SANDBOX"
        progress.status = ci.TaskletProgress.Status.RUNNING
        return progress

    def _spawn_new_branch(self, revision, c_info, branch_no) -> bool:
        rm_response = self.rm_client.post_pre_release(revision,
                                                      c_info.testenv_cfg__trunk_db,
                                                      self.input.config.component_name,
                                                      branch_no)
        logging.info('RM response: %s', rm_response)
        return rm_response['status'] == 200

    def _get_resources(self, branch_no, revision, resource_names_list):
        task_id = self._wait_for(self._get_build_task_for_revision, branch_no, revision)
        result = dict()
        if task_id == 0:
            return result
        for res in resource_names_list:
            result[res] = self._wait_for(self._get_resource_id_from_sb_task, task_id, res)
        return result

    def _get_first_commit_id(self, component_name, branch_no):
        branch = self.rm_client.get_scopes(component_name, 1, start_scope_number=branch_no)
        try:
            tag_info = self._get_current_scope(branch['branchScopes'], branch_no)
            self.working_rm_info['tag'] = int(tag_info['tagNumber'])
            self.working_rm_info['base_commit_id'] = int(tag_info['baseCommitId'])
            return tag_info['firstCommitId']
        except KeyError:
            logging.error('RM is not up yet')
            return None

    @staticmethod
    def _get_current_scope(scopes, branch_number):
        for scope in scopes:
            if int(scope['scopeNumber']) == branch_number:
                return scope['tags'][0]
        raise KeyError

    def _get_build_task_for_revision(self, current_branch, revision) -> Optional[int]:
        events = self.rm_client.get_events(self.input.config.component_name, 'BuildTest')
        for event in events['events']:
            if event['event']['buildTestData']['scopeNumber'] == str(current_branch) and \
               event['event']['buildTestData']['revision'] == str(revision):
                logging.info("EventData: %s", event['event'])
                return int(event['event']['taskData']['taskId'])
        logging.info('Can\'t find build task for branch=%s, revision=%s', current_branch, revision)
        return None

    @staticmethod
    def _get_resource_id_from_sb_task(task_id, resource_name):
        logging.info('Looking for Task: %s and Resource: %s', task_id, resource_name)
        resource = sdk2.Resource.find(type=resource_name, task_id=task_id).first()
        return resource.id

    def _wait_for(self, fun, *args):
        retry_counter = 0
        sleep_for = self.retries_config['sleep_time']
        while retry_counter < self.retries_config['retries']:
            result = fun(*args)
            if result is not None:
                return result
            logging.info('Data is not ready yet. Sleeping for %d seconds (retry %d)', sleep_for, retry_counter)
            time.sleep(sleep_for)
            sleep_for = sleep_for * self.retries_config['backoff']
            retry_counter += 1
        raise common.errors.TaskError('Data collection took too long')
