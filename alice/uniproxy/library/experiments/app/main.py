from alice.uniproxy.library.experiments import Experiments
from alice.uniproxy.library.events import Event
from alice.uniproxy.library.utils.experiments import safe_experiments_vins_format
import json
import sys


class FakeUniSystem:
    def __init__(self):
        self.patcher = None
        self.uaas_flags = {}
        self.uaas_test_ids = []
        self.session_data = None
        self.exps_check = False

    def set_event_patcher(self, patcher):
        self.patcher = patcher

    def log_experiment(self, *args, **kwargs):
        pass

    def patch(self, event):
        self.patcher.patch(event, self.session_data)


def main():
    import argparse
    
    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--experiments", help='port to listen on')
    parser.add_argument("-m", "--macros", help='processes count')
    args = parser.parse_args()

    with open(args.experiments, "r") as f:
        experiment_list = json.load(f)
    with open(args.macros, "r") as f:
        macro_list = json.load(f)
    
    experiments = Experiments(experiment_list, macro_list, mutable_shares=True)

    us = FakeUniSystem()
    for l in sys.stdin.readlines():
        event = Event(json.loads(l))

        request = event.payload.get('request')
        if request:
            if "experiments" in request:
                request["experiments"] = safe_experiments_vins_format(
                    request['experiments'], lambda *args, **kwargs: None
                )
        if not us.session_data:
            us.session_data = event.payload
            experiments.try_use_experiment(us)
        us.patch(event)
        print(json.dumps({
            "header": {
                "name": event.name,
                "namespace": event.namespace,
                "messageId": event.message_id
            },
            "payload": event.payload
        }), flush=True)


if __name__ == "__main__":
    main()
