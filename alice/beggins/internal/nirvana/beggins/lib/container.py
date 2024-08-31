import os

import attr


@attr.s(slots=True, frozen=True)
class Inputs:
    script = attr.ib(default=os.getenv('SOURCE_CODE_PATH', './input_script'))
    data = attr.ib(default=os.getenv('INPUT_PATH', './input_data'))

    def script_file(self, filepath):
        return os.path.join(self.script, filepath)

    def data_file(self, filepath):
        return os.path.join(self.data, filepath)


@attr.s(slots=True, frozen=True)
class Outputs:
    data = attr.ib(default=os.getenv('TMP_OUTPUT_PATH', './output_data'))
    state = attr.ib(default=os.getenv('SNAPSHOT_PATH', './output_state'))
    logs = attr.ib(default=os.getenv('LOGS_PATH', './output_logs'))
    json_output = attr.ib(default=os.getenv('JSON_OUTPUT_FILE', 'output.json'))

    def data_file(self, filepath):
        return os.path.join(self.data, filepath)

    def state_file(self, filepath):
        return os.path.join(self.state, filepath)

    def logs_file(self, filepath):
        return os.path.join(self.logs, filepath)


INPUT = Inputs()
OUTPUT = Outputs()

FORMAT_MAP = {
    'input/script': INPUT.script,
    'input/data': INPUT.data,

    'output/data': OUTPUT.data,
    'output/state': OUTPUT.state,
    'output/logs': OUTPUT.logs,
}
