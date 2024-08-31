# coding=utf-8
import os
import sys
import argparse
import yt.yson as yson
import yt.wrapper as yt
import hashlib
from typing import Optional

yt.config.set_proxy("hahn.yt.yandex.net")


GC_INTENT_NAME = 'personal_assistant\tgeneral_conversation\tgeneral_conversation'
PURE_GC_INTENT_NAME = 'personal_assistant\tscenarios\tpure_general_conversation'
EXTERNAL_SKILL_CONTINUE_INTENT_NAME = 'personal_assistant\tscenarios\texternal_skill_gc\tcontinue'
EXTERNAL_SKILL_INTENT_NAME = 'personal_assistant\tscenarios\texternal_skill_gc'
MM_GC_INTENT_NAME = 'personal_assistant\tscenarios\tvinsless\tgeneral_conversation'
DEFAULT_LANG = 'ru'


@yt.yt_dataclass
class PreparedLogsExpboxesRow:
    other: Optional[yt.schema.YsonBytes]
    uuid: Optional[str]
    icookie: Optional[str]
    puid: Optional[str]
    user_id: Optional[str]
    cohort: Optional[str]
    is_new: Optional[str]
    first_day: Optional[str]
    app: Optional[str]
    platform: Optional[str]
    version: Optional[str]
    country_id: Optional[int]
    is_exp_changed: Optional[bool]
    session_id: Optional[str]
    session_sequence: Optional[int]
    req_id: Optional[str]
    fielddate: Optional[str]
    testids: Optional[yt.schema.YsonBytes]
    expboxes: Optional[str]
    query: Optional[str]
    reply: Optional[str]
    intent: Optional[str]
    generic_scenario: Optional[str]
    mm_scenario: Optional[str]
    skill_id: Optional[str]
    input_type: Optional[str]
    client_time: Optional[int]
    server_time_ms: Optional[int]
    alice_speech_end_ms: Optional[int]
    is_interrupted: Optional[bool]
    child_confidence: Optional[float]
    device_id: Optional[str]
    device_revision: Optional[str]
    device: Optional[str]
    is_tv_plugged_in: Optional[bool]
    screen: Optional[str]
    sound_level: Optional[float]
    sound_muted: Optional[bool]
    analytics_info: Optional[yt.schema.YsonBytes]
    music_answer_type: Optional[str]
    music_genre: Optional[str]
    voice_text: Optional[str]
    is_smart_home_user: Optional[bool]
    client_tz: Optional[str]
    location: Optional[yt.schema.YsonBytes]
    subscription: Optional[str]
    parent_req_id: Optional[str]
    parent_scenario: Optional[str]
    request_act_type: Optional[str]
    trash_or_empty_request: Optional[bool]
    lang: Optional[str] = DEFAULT_LANG
    do_not_use_user_logs: Optional[bool] = False


@yt.yt_dataclass
class GeneralConversationRow:
    session_id: str
    session_sequence: Optional[int]
    req_id: str
    gc_source: Optional[str]
    query: Optional[str]
    reply: str
    app: str
    date: str
    delta: Optional[int]
    gc_classifier_score: Optional[float]
    intent: str
    input_type: Optional[str]
    gc_intent: Optional[str]


@yt.yt_dataclass
class SessionRow:
    session_id: str
    req_id: str
    gc_source: Optional[str]
    context: list[Optional[str]]
    reply: str
    app: str
    date: str
    deltas: list[Optional[int]]
    gc_classifier_score: Optional[float]
    hash_code: str
    intent: str
    intents: list[Optional[str]]
    is_external_skill: bool
    is_reply_empty: bool
    sources: list[Optional[str]]
    turn_idx: int
    postintent: Optional[str]
    postreply: Optional[str]
    postdelta: Optional[int]


@yt.with_context
class HeadMapper(object):
    def __init__(self, top_size):
        self.top_size = top_size

    def __call__(self, row, context):
        if context.row_index >= self.top_size:
            return
        yield row


class HeadReducer(object):
    def __init__(self, top_size):
        self.top_size = top_size

    def __call__(self, rows):
        for row_idx, row in enumerate(rows):
            if row_idx >= self.top_size:
                return
            yield row


def to_serializable_types(obj):
    if isinstance(obj, yson.yson_types.YsonEntity):
        return None
    return obj


class DialogueBuilderMapper(yt.TypedJob):
    def __init__(self, num_tables, apps, lang, shown_queries, test_ids):
        self.num_tables = num_tables
        self.apps = apps
        self.lang = lang
        self.shown_queries = shown_queries
        self.test_ids = test_ids

    def prepare_operation(self, context, preparer):
        for table_idx in range(self.num_tables):
            preparer.input(table_idx, type=PreparedLogsExpboxesRow)
        preparer.output(0, type=GeneralConversationRow)

    def __call__(self, row):
        row_test_ids = yson.loads(row.testids)
        row_other = yson.loads(row.other)
        row_analytics_info = yson.loads(row.analytics_info).get("analytics_info", None)
        if self.apps and row.app not in self.apps:
            return
        if not row.lang.startswith(self.lang):
            return
        if self.test_ids and not any(test_id in self.test_ids for test_id in row_test_ids):
            return
        if row.do_not_use_user_logs:
            return

        if row_analytics_info is None:
            return

        try:
            gc_score = row_analytics_info['GeneralConversation']['scenario_analytics_info']['objects'][0]['gc_response_info'].get('gc_classifier_score', None)
        except KeyError:
            gc_score = None
            
        gc_source = to_serializable_types(row_other.get('gc_source', None))
        gc_intent = to_serializable_types(row_other.get('gc_intent', None))
        reply = row.reply if row.reply is not None else ""

        query = row_analytics_info.get('shown_utterance') if self.shown_queries else row.query

        yield GeneralConversationRow(
            session_id=row.session_id,
            session_sequence=row.session_sequence,
            req_id=row.req_id,
            gc_source=gc_source,
            query=query,
            reply=reply,
            app=row.app,
            date=row.fielddate,
            delta=row_other['delta'],
            gc_classifier_score=gc_score,
            intent=row.intent,
            input_type=row.input_type,
            gc_intent=gc_intent,
        )


class DialogueBuilderReducer(yt.TypedJob):
    def __init__(self, context_length, lang, postreply, salt, scenario):
        self.context_length = context_length
        self.lang = lang
        self.postreply = postreply
        self.salt = salt
        self.scenario = scenario

    def prepare_operation(self, context, preparer):
        preparer.input(0, type=GeneralConversationRow).output(0, type=SessionRow)

    def _is_gc_reply(self, turn, was_pure_gc_entry, next_intent, does_session_start_in_gc_skill):
        if turn.reply == 'EMPTY' or not turn.reply:
            return False
        if self.lang == 'tr':
            cond = turn.intent == MM_GC_INTENT_NAME
        else:
            cond = (turn.intent == GC_INTENT_NAME) or \
                ((turn.gc_intent == GC_INTENT_NAME) and
                    (was_pure_gc_entry or turn.intent == EXTERNAL_SKILL_CONTINUE_INTENT_NAME or (does_session_start_in_gc_skill and next_intent != EXTERNAL_SKILL_CONTINUE_INTENT_NAME)))
        return cond

    def __call__(self, rows):
        rows = list(rows)
        was_pure_gc_entry = False
        does_session_start_in_gc_skill = False
        utterances = [None] * (self.context_length + 1)
        sources = [None] * (self.context_length // 2 + 1)
        intents = [None] * (self.context_length // 2 + 1)
        deltas = [None] * (self.context_length // 2 + 1)
        for turn_idx, turn in enumerate(rows):
            utterances = (utterances + [turn.query, turn.reply])[2:]
            sources = (sources + [turn.input_type])[1:]
            intents = (intents + [turn.intent])[1:]
            deltas = (deltas + [turn.delta])[1:]
            was_pure_gc_entry |= turn.gc_intent == PURE_GC_INTENT_NAME
            does_session_start_in_gc_skill |= (turn_idx == 0 and turn.intent == EXTERNAL_SKILL_INTENT_NAME)
            if turn_idx + 1 < len(rows):
                next_intent = rows[turn_idx + 1].intent
                next_reply = rows[turn_idx + 1].query
                next_delta = rows[turn_idx + 1].delta
            else:
                next_intent = None
                next_reply = None
                next_delta = None
            if self.scenario == 'gc' and not self._is_gc_reply(turn, was_pure_gc_entry, next_intent, does_session_start_in_gc_skill):
                continue
            hash_code = hashlib.md5('\t'.join([turn.req_id, rows[0].session_id, str(turn_idx), rows[0].date, self.salt]).encode()).hexdigest()
            yield SessionRow(
                session_id=turn.session_id,
                req_id=turn.req_id,
                gc_source=turn.gc_source,
                context=utterances[:-1],
                reply=turn.reply,
                app=turn.app,
                date=turn.date,
                deltas=deltas,
                gc_classifier_score=turn.gc_classifier_score,
                hash_code=hash_code,
                intent=turn.intent,
                intents=intents,
                is_external_skill=turn.intent != GC_INTENT_NAME,
                is_reply_empty=turn.reply == 'EMPTY',
                sources=sources,
                turn_idx=turn_idx,
                postintent=next_intent if self.postreply else None,
                postreply=next_reply if self.postreply else None,
                postdelta=next_delta if self.postreply else None,
            )


def main(args):
    all_dates = yt.list(args.src, absolute=False)
    from_date = args.from_date if args.from_date else min(all_dates)
    to_date = args.to_date if args.to_date else max(all_dates)
    dates = [date for date in all_dates if from_date <= date <= to_date]
    tables = [os.path.join(args.src, date) for date in dates]
    print(dates, file=sys.stderr)
    assert tables
    stratify_by = args.stratify_by.split(',') if args.stratify_by else []
    apps = set(args.apps.split(',')) if args.apps else None
    test_ids = set(args.test_ids.split(',')) if args.test_ids else None
    salt = '\t'.join([args.salt, from_date, to_date])

    if yt.exists(args.dst):
        yt.remove(args.dst)

    yt.run_map_reduce(
        DialogueBuilderMapper(len(tables), apps, args.lang, args.shown_queries, test_ids),
        DialogueBuilderReducer(args.context_length, args.lang, args.postreply, salt, args.scenario),
        tables, args.dst, reduce_by='session_id', sort_by=["session_id", "session_sequence"],
        spec={"pool": args.pool},
    )

    if stratify_by:
        assert args.num_samples
        yt.run_map_reduce(
            None, HeadReducer(args.num_samples), args.dst, args.dst, reduce_by=stratify_by,
            sort_by=stratify_by+['hash_code'], spec={'job_io': {'control_attributes': {'enable_row_index': True}}}
        )
    elif args.num_samples or args.ratio:
        yt.run_sort(args.dst, sort_by='hash_code')
        num_samples = args.num_samples if args.num_samples else yt.row_count(args.dst) * args.ratio
        yt.run_map(
            HeadMapper(num_samples), args.dst, yt.TablePath(args.dst),
            spec={'job_io': {'control_attributes': {'enable_row_index': True}}}
        )

    yt.run_sort(args.dst, sort_by=stratify_by+['req_id'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', default='//home/alice/dialog/prepared_logs_expboxes', help="Log source; yt folder with tables.")
    parser.add_argument('--from-date', help="Date to start from. Format: yyyy-mm-dd")
    parser.add_argument('--to-date', help="Date to end at. Format: yyyy-mm-dd")
    parser.add_argument('--dst', required=True, help="Destination table path.")
    parser.add_argument('--context-length', type=int, default=9, help="Length of dialog context without the reply.")
    parser.add_argument('--num-samples', type=int, help="Number of samples to take.")
    parser.add_argument('--ratio', type=float, help="Ratio of samples to take.")
    parser.add_argument('--stratify-by', help='comma separated columns')    
    parser.add_argument('--apps', help="Comma separated list of apps to include. Default: all.")
    parser.add_argument('--lang', default='ru', help="Language of the data. Default: ru.")
    parser.add_argument('--postreply', action='store_true', help="Include user reply after Alice one.")
    parser.add_argument('--shown-queries', action='store_true')
    parser.add_argument('--salt', default='', help="Salt for hash function")
    parser.add_argument('--scenario', choices=['gc', 'all'], default='gc', help="Logged scenaries. Choises: gc, all.")
    parser.add_argument('--test-ids', help="Comma separated list of test ids from ab experiments to include. Default: all.")
    parser.add_argument('--pool', default="dialogs", help="Pool to use for YT.")
    args = parser.parse_args()
    assert args.num_samples is None or args.ratio is None
    assert args.ratio is None or 0. < args.ratio < 1.
    assert args.num_samples is None or args.num_samples > 0
    main(args)
