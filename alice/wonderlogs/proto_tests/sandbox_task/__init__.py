import json
import logging
import os
import subprocess

from sandbox import sdk2
from sandbox.common.errors import TaskError
from sandbox.projects.common.vcs import arc


class CheckWonderlogsProtos(sdk2.Task):
    class Parameters(sdk2.Task.Parameters):

        pr = sdk2.parameters.Integer('PR ID', required=True)

        protoc = sdk2.parameters.Resource('Protoc', required=True)

        proto_tester = sdk2.parameters.Resource('Proto tester', required=True)

    def _run_protoc(self, arcadia_root, image_name):
        return subprocess.run(
            [str(sdk2.ResourceData(self.Parameters.protoc).path),
             os.path.join(arcadia_root, 'alice/wonderlogs/protos/wonderlogs.proto'),
             '--descriptor_set_out', os.path.join(sdk2.paths.get_logs_folder(), image_name),
             '--proto_path', os.path.join(arcadia_root, 'contrib/libs/protobuf/src'),
             '--proto_path', arcadia_root,
             '--include_imports'],
            capture_output=True)

    @staticmethod
    def _log_process(process, name):
        logging.info(f'{name} process code: {process.returncode}')
        logging.error(f'{name} process stderr: {process.stderr}')
        logging.error(f'{name} process stdout: {process.stdout}')

    def on_execute(self):
        arc_client = arc.Arc()
        proto_tester = str(sdk2.ResourceData(self.Parameters.proto_tester).path)
        with arc_client.mount_path(None, None, fetch_all=False) as trunk:
            process = self._run_protoc(trunk, 'trunk_image.bin')
            self._log_process(process, 'Image of trunk proto')

            if process.returncode:
                raise TaskError(f'Failed getting trunk proto image of protos, stderr: "{process.stderr}"')

        with arc_client.mount_path(None, None, fetch_all=False) as branch:
            arc_client.pr_checkout(branch, self.Parameters.pr)
            arc_client.rebase(branch, 'trunk')
            process = self._run_protoc(branch, 'branch_image.bin')
            self._log_process(process, 'Image of branch proto')

            if process.returncode:
                raise TaskError(f'Failed getting branch proto image of protos, stderr: "{process.stderr}"')

        process = subprocess.run(
            [proto_tester,
             '--old-image', os.path.join(sdk2.paths.get_logs_folder(), 'trunk_image.bin'),
             '--new-image', os.path.join(sdk2.paths.get_logs_folder(), 'branch_image.bin')],
            capture_output=True)

        self._log_process(process, 'Proto checking')

        result = json.loads(process.stdout)
        self.Context.report = 'If you have any questions, feel free to write to @ran1s.'

        if process.returncode:
            raise TaskError(f'Failed checking protos, returncode: {process.returncode}, stderr: "{process.stderr}"')

        if not result['passed']:
            raise TaskError(f'Failed checking protos, errors:\n{result["message"]}')

    @sdk2.footer()
    def report(self):
        return self.Context.report
