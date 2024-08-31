import json
import logging

from google.protobuf import text_format, json_format

from alice.hollywood.library.python.testing.integration.test_functions import prepare_canondata_result

logger = logging.getLogger(__name__)


class ScenarioResponses:
    def __init__(self, run_response=None, continue_response=None, apply_response=None, commit_response=None, sources_dump=None):
        self.run_response = run_response
        self.continue_response = continue_response
        self.apply_response = apply_response
        self.commit_response = commit_response
        self.sources_dump = sources_dump

    def __str__(self):
        return prepare_canondata_result(run_response=self.run_response,
                                        continue_response=self.continue_response,
                                        apply_response=self.apply_response,
                                        commit_response=self.commit_response)

    def scenario_stages(self):
        result = set()
        if self.run_response:
            result.add('run')
        if self.continue_response:
            result.add('continue')
        if self.apply_response:
            result.add('apply')
        if self.commit_response:
            result.add('commit')
        return result

    def run_error(self):
        return self.run_response.Error if self.run_response.HasField('Error') else None

    def is_run_irrelevant(self):
        return self.run_response.Features.IsIrrelevant

    def is_run_relevant(self):
        return not self.run_error() and not self.is_run_irrelevant()

    def is_run_relevant_with_second_scenario_stage(self, second_scenario_stage=None):
        if self.is_run_irrelevant():
            return False
        if second_scenario_stage:
            return self.scenario_stages() == {'run', second_scenario_stage}
        else:
            return self.scenario_stages() == {'run'}

    @property
    def run_response_pyobj(self):
        if not self.run_response:
            return None
        result = proto_to_pyobj(self.run_response)
        logger.info(f'Hollywood run_response as a Python object: {result}')
        return result

    @property
    def continue_response_pyobj(self):
        if not self.continue_response:
            return None
        result = proto_to_pyobj(self.continue_response)
        logger.info(f'Hollywood continue_response as a Python object: {result}')
        return result

    @property
    def apply_response_pyobj(self):
        if not self.apply_response:
            return None
        result = proto_to_pyobj(self.apply_response)
        logger.info(f'Hollywood apply_response as a Python object: {result}')
        return result

    @property
    def commit_response_pyobj(self):
        if not self.commit_response:
            return None
        result = proto_to_pyobj(self.commit_response)
        logger.info(f'Hollywood commit_response as a Python object: {result}')
        return result


class Accumulator:
    def __init__(self):
        self.items = []

    def add(self, item, label=None):
        self.items.append((str(item), label))

    def add_proto(self, item, label=None):
        self.items.append((proto_to_text(item), label))

    def __str__(self):
        result = ''
        for index, (item_str, label) in enumerate(self.items):
            result += '##################\n'
            result += f'# Dialog phrase {index}'
            if label:
                result += f', {label}\n'
            else:
                result += '\n'
            result += item_str
            result += '\n'
        return result


def proto_to_text(proto):
    return text_format.MessageToString(proto, as_utf8=True)


def proto_to_pyobj(proto):
    return json.loads(json_format.MessageToJson(proto))


def proto_to_json(proto):
    return json.dumps(proto_to_pyobj(proto), indent=4, sort_keys=True, ensure_ascii=False)
