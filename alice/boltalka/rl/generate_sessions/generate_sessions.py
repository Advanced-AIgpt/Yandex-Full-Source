import os
import argparse
import yt.wrapper as yt
from alice.boltalka.py_libs.apply_nlg_dssm.apply_nlg_dssm import NlgDssmApplier
yt.config.set_proxy("hahn.yt.yandex.net")

MODEL_FNAME = 'data/alice_filtering.model'
GC_INTENT_NAME = 'personal_assistant\tgeneral_conversation\tgeneral_conversation'
GC_SKILL_ID = 'bd7c3799-5947-41d0-b3d3-4a35de977111'


class DevicesFilter(object):
    def __init__(self, devices):
        self.devices = set(devices)

    def __call__(self, row):
        return row['app'] in self.devices


class SessionGenerator(object):
    def __init__(self, devices_filter=None):
        self.devices_filter = devices_filter

    def start(self):
        self.alice_filtering_model = NlgDssmApplier(os.path.basename(MODEL_FNAME))
        pass

    def get_matchers(self, row):
        if 'pure_general_conversation' in row['experiments']:
            return (
                lambda x: x['intent'] ==
                'personal_assistant\tscenarios\texternal_skill_gc' and x[
                    'gc_intent'] ==
                'personal_assistant\tscenarios\tpure_general_conversation',
                lambda x: x['intent'] ==
                'personal_assistant\tscenarios\texternal_skill_gc' and x[
                    'gc_intent'] !=
                'personal_assistant\tscenarios\tpure_general_conversation',
                lambda x: x['intent'] ==
                'personal_assistant\tscenarios\tpure_general_conversation_deactivation'
            )
        else:
            return (
                lambda x: x['intent'] ==
                'personal_assistant\tscenarios\texternal_skill_gc',
                lambda x: x['intent'] ==
                'personal_assistant\tscenarios\texternal_skill_gc\tcontinue',
                lambda x: x['intent'] ==
                'personal_assistant\tscenarios\texternal_skill_gc\tdeactivate')

    def get_alice_self_play_score(self, session):
        if len(session) < 5 or len(session) > 300:
            return 0.
        CONTEXT_SIZE = 3
        utterances = sum([[el['query'], el['reply']] for el in session], [])
        user_contexts = [
            utterances[i - CONTEXT_SIZE:i]
            for i in range(2, len(utterances), 2)
        ]
        user_replies = utterances[2::2]
        scores = self.alice_filtering_model.get_scores(user_contexts,
                                                       user_replies)
        return sum(scores) / len(scores)

    def __call__(self, row):
        if self.devices_filter is not None and not self.devices_filter(row):
            return

        session = []
        feedback = -1
        in_gc = False
        is_activate, is_continue, is_deactivate = self.get_matchers(row)
        for turn in row["session"] + ['last_turn']:
            if turn == 'last_turn' or is_activate(turn):
                if session:
                    length = len(session)
                    yield {
                        "session":
                        session,
                        "length":
                        length,
                        "feedback":
                        feedback,
                        "uuid":
                        row["uuid"],
                        "date":
                        row["fielddate"],
                        "alice_self_play_score":
                        self.get_alice_self_play_score(session),
                        "device":
                        row["app"]
                    }
                in_gc = True
                session = []
                feedback = -1
            elif in_gc:
                if is_continue(turn):
                    if turn["type"] not in ["voice", "text"]:
                        in_gc = False
                        session = []
                        feedback = -1
                        continue
                    session.append(
                        dict(
                            query=turn['_query'] or '',
                            reply=turn['_reply'] or '',
                            is_gc=turn['gc_intent'] ==
                            'personal_assistant\tgeneral_conversation\tgeneral_conversation',
                            gc_source=turn.get('gc_source', None)
                        ))
                elif is_deactivate(turn):
                    in_gc = False
                else:
                    in_gc = False
                    session = []
                    feedback = -1
                    continue
            elif turn["intent"].startswith(
                    "personal_assistant\tfeedback\tgc_feedback"):
                feedback = 0
                if turn["intent"].endswith("neutral"):
                    feedback = 1
                elif turn["intent"].endswith("positive"):
                    feedback = 2


def run(args):
    srcs = []
    lower_bound = os.path.join(args.src, args.from_date)
    upper_bound = os.path.join(args.src, args.to_date)
    for table in yt.list(args.src, absolute=True):
        if lower_bound <= table <= upper_bound:
            srcs.append(table)
    yt.run_map(SessionGenerator(DevicesFilter(args.devices.split(',')) if args.devices is not None else None),
               srcs,
               args.dst,
               local_files=[MODEL_FNAME],
               memory_limit=20 * 1024**3)
    yt.run_merge(
        [args.dst],
        args.dst,
        spec={'combine_chunks': True}
    )


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', default='//home/voice/dialog/sessions')
    parser.add_argument('--from-date', required=True)
    parser.add_argument('--to-date', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--devices', type=str, default=None, help='Devices names comma separated')
    args = parser.parse_args()
    run(args)
