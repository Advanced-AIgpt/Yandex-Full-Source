import json
import logging
import os.path

from alice.megamind.tests.library.settings import Settings
from alice.joker.library.python import JokerSyncRequest


logger = logging.getLogger(__name__)


class SessionManager:
    def __init__(self, stubs_info_dir):
        self.stubs_info_dir = stubs_info_dir
        self.settings = Settings()

    def get_metadata(self, test_name):
        with open(self._filename(test_name), 'r') as f:
            return json.load(f)

    def set_metadata(self, test_name, data):
        with open(self._filename(test_name), 'w') as f:
            return json.dump(data, f)

    def _filename(self, test_name):
        return os.path.join(self.stubs_info_dir, test_name + '.meta.json')

    def test_base_path(self, test_name):
        return os.path.join(self.stubs_info_dir, test_name)

    def sync_stubs(self, joker, test_name):
        filename = self.test_base_path(test_name) + '.txt'
        try:
            sync_request = JokerSyncRequest(joker)
            with open(filename, 'r') as f:
                linenum = 0
                for line in f:
                    linenum += 1
                    version, stub_data = line.rstrip().split(sep='\t', maxsplit=1)
                    if version != '1':
                        raise JokerSyncRequest.Exception("Unsupported version of stub in file '{}:{}'".format(filename, linenum))

                    sync_request.add_stub(stub_data)

            sync_request.check_run()
        except FileNotFoundError as e:
            logger.info('unable to open file (%s) for sync stubs: %s', filename, e)
