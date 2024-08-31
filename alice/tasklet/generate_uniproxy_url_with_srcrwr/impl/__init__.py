import re
import logging
from alice.tasklet.generate_uniproxy_url_with_srcrwr.proto import generate_uniproxy_url_with_srcrwr_tasklet
from sandbox import common

VALID_PATTERN = r'^[a-zA-Z0-9-_.:]*$'


class GenerateUniproxyUrlWithSrcrwrImpl(generate_uniproxy_url_with_srcrwr_tasklet.GenerateUniproxyUrlWithSrcrwrBase):
    def run(self):
        logging.info("Generating url for uniproxy")
        try:
            self._validate_input()
        except ValueError:
            raise common.errors.TaskFailure('Source and destination must match ' + VALID_PATTERN)
        srcrwrs = '&'.join([f'srcrwr={x.source}:{x.destination}' for x in self.input.data.srcrwr])
        self.output.state.generated_url = f'{self.input.data.base_url}?{srcrwrs}'
        self.output.state.success = True

    def _validate_input(self):
        for srcrwr in self.input.data.srcrwr:
            if not re.match(VALID_PATTERN, srcrwr.source) or not re.match(VALID_PATTERN, srcrwr.destination):
                raise ValueError
