# -*-coding: utf8 -*-
from math import log

from nile.api.v1 import (
    clusters,
    Record,
    filters as nf,
    statface as ns,
    aggregators as na,
    extractors as ne,
    multischema, with_hints, modified_schema
)

from itertools import product
from utils.nirvana.op_caller import call_as_operation
from utils.statface.nile_client import make_stat_client

FIELDS_AS_SLICES = ['app', 'platform', 'version', 'is_new', 'cohort']


def multiply_records(fixed_data, data_to_multiply, stream):
    """
        yields data into stream for every possible subset of values in data_to_multiply
        changed to "_total_". Example:
        fixed_data = {'field0':'value0'}
        data_to_multiply = {'field1':'value1', 'field2':'value2', 'field3':'value3'}

        in the stream will be:
        Record('field0':'value0', 'field1':'value1', 'field2':'value2', 'field3':'value3')
        Record('field0':'value0', 'field1':'_total_', 'field2':'value2', 'field3':'value3')
        Record('field0':'value0', 'field1':'value1', 'field2':'_total_', 'field3':'value3')
        Record('field0':'value0', 'field1':'value1', 'field2':'value2', 'field3':'_total_')
        Record('field0':'value0', 'field1':'_total_', 'field2':'_total_', 'field3':'value3')
        Record('field0':'value0', 'field1':'_total_', 'field2':'value2', 'field3':'_total_')
        Record('field0':'value0', 'field1':'value1', 'field2':'_total_', 'field3':'_total_')
        Record('field0':'value0', 'field1':'_total_', 'field2':'_total_', 'field3':'_total_')

        fixed_data and data_to_multiply are python dicts
        stream is a callable which takes a Record argument
        (for example - a nile stream instance from custom map with multiple outputs)
    """
    fields_to_multiply = data_to_multiply.keys()

    data_to_multiply_d = {'_total_': '_total_'}
    data_to_multiply_d.update(data_to_multiply)

    for fields in product(*product(fields_to_multiply, ['_total_'])):
        new_data = dict(zip(fields_to_multiply, map(data_to_multiply_d.get, fields)))
        new_data.update(**fixed_data)
        stream(Record(**new_data))


def get_request_response_type(intent):
    if ('pure_general_conversation_deactivation' not in intent and 'general_conversation' in intent) \
            or ('handcrafted' in intent and 'feedback' not in intent):
        return 'general_conversation'
    elif 'external_skill_gc' in intent and 'deactivate' not in intent:
        return 'external_skill_gc'
    elif 'gc_feedback' in intent:
        return 'gc_feedback'
    elif 'feedback' in intent:
        # notice that there is a 'handcrafted' feedback,
        # which is assumed to be part of general_conversation
        return 'maybe_general_conversation_feedback'
    else:
        return None


def get_len_safe(maybe_string):
    return len(maybe_string.split()) if maybe_string is not None else 0


@with_hints(output_schema=multischema(
    modified_schema(extend={'dialog_length': int, 'dialog_log_length': float,
                            'dialog_duration': int, 'dialog_log_duration': float,
                            'gc_type': str, 'feedback': str},
                    exclude=['session']),
    modified_schema(extend={'reply_length': int, 'query_length': int,
                            'delta': int, 'log_delta': float,
                            'gc_type': str, 'feedback': str},
                    exclude=['session'])), outputs_count=2)
def split_general_conversation_and_gc(records,
                                      dialog_stats_stream,
                                      individual_record_stats_stream):
    """
        split stream into two parts - stats on dialog level and stats on individual request level
    """

    def get_dialog_stats(dialog):
        dialog_len = len(dialog)
        dialog_log_len = log(dialog_len + 1.)
        dialog_duration = sum(filter(None, [req.get('delta', 0) for req in dialog[1:]])) #first req shouldn't be counted
        dialog_log_duration = log(dialog_duration + 1.)
        return {'dialog_length': dialog_len,
                'dialog_log_length': dialog_log_len,
                'dialog_duration': dialog_duration,
                'dialog_log_duration': dialog_log_duration}

    def get_individual_record_stats(request_response, fielddate):

        # do not use request_response.get('delta', 0), since it will break
        # things in case of request_response actually having a key 'delta' with None value
        delta = request_response.get('delta') if request_response.get('delta') is not None else 0

        return {'reply_length': get_len_safe(request_response.get('reply', '')),
                'query_length': get_len_safe(request_response.get('query', '')),
                'delta': delta,
                'log_delta': log(delta + 1),
                'fielddate': fielddate}

    for record in records:
        # a list for either external_skill_gc or general conversation
        # (those dialogs can not intersect)
        # general_conversation dialog is an uninterrupted sequence of requests
        # with intent either 'general_conversation' or 'handcrafted'
        # external_skill_gc dialog is ... with intents 'external_skill_gc',
        # except intent "external_skill_gc\tdeactivate"
        current_gc_dialog = []

        # init variable to keep track of which kind of dialog we are constructing
        current_dialog_type = None

        external_skill_gc_dialogs = []
        general_conversation_dialogs = []

        for req in record['session']:

            request_response_type = get_request_response_type(req.get('intent', ''))

            if current_dialog_type is None:
                # in case of first entry in sessions or in case when
                # previous entrys' intent returned None from get_request_response_type
                if request_response_type == 'external_skill_gc' or request_response_type == 'general_conversation':
                    # if one of the dialogs, append to current (empty) list and save its type
                    current_gc_dialog.append(req)
                    current_dialog_type = request_response_type
                elif request_response_type == 'gc_feedback':
                    # if we see gc_feedback, save it to latest external_skill_gc_dialog
                    try:
                        external_skill_gc_dialogs[-1]["feedback"] = req.get('intent').split('\t')[-1][len('gc_'):]
                    except:
                        if len(external_skill_gc_dialogs) >= 1:
                            external_skill_gc_dialogs[-1]["feedback"] = '_no_feedback_'
                # end if
            elif current_dialog_type == 'external_skill_gc':
                # in case previously was request_response pair of type external_skill_gc
                if request_response_type == 'external_skill_gc':
                    # easy case, just append another external_skill_gc request_response pair
                    current_gc_dialog.append(req)
                elif request_response_type is None or request_response_type == 'maybe_general_conversation_feedback':
                    # external_skill_gc is finished. Save resulting dialog and reset current_gc_dialog
                    external_skill_gc_dialogs.append({'dialog': current_gc_dialog,
                                                      'fielddate': record['fielddate'],
                                                      'feedback': '_no_feedback_'})
                    current_gc_dialog = []
                    current_dialog_type = None
                elif request_response_type == 'gc_feedback':
                    # dialog ended AND feedback received. Save both

                    external_skill_gc_dialogs.append({'dialog': current_gc_dialog,
                                                      'fielddate': record['fielddate'],
                                                      'feedback': '_no_feedback_'})
                    try:
                        external_skill_gc_dialogs[-1]['feedback'] = req.get('intent').split('\t')[-1][len('gc_'):]
                    except:
                        pass

                    current_gc_dialog = []
                    current_dialog_type = None
                elif request_response_type == 'general_conversation':
                    # actually shouldn't occur, but possible (normally we will see intent "deactivate"
                    # at the end of external_skill_gc dialog)
                    # save finished dialog and create a new one of different type
                    external_skill_gc_dialogs.append({'dialog': current_gc_dialog,
                                                      'fielddate': record['fielddate'],
                                                      'feedback': '_no_feedback_'})
                    current_gc_dialog = [req]
                    current_dialog_type = 'general_conversation'
                # end if - no "else" since there are no other possibilities
            elif current_dialog_type == 'general_conversation':
                # in case previously was request_response pair of type general_conversation
                if request_response_type == 'general_conversation':
                    # easy case, just append another general_conversation request_response pair
                    current_gc_dialog.append(req)
                elif request_response_type == 'maybe_general_conversation_feedback':
                    # the case when general_feedback arrives right after general_conversation
                    # and so we assume it is related to it. Current dialog assumed to be finished
                    # In all other cases maybe_general_conversation_feedback is ignored
                    general_conversation_dialogs.append({'dialog': current_gc_dialog,
                                                         'fielddate': record['fielddate'],
                                                         'feedback': '_no_feedback_'})
                    try:
                        general_conversation_dialogs[-1]['feedback'] = req.get('intent').split('\t')[-1]
                    except:
                        pass

                    current_gc_dialog = []
                    current_dialog_type = None
                elif request_response_type == 'external_skill_gc':
                    # if general conversation is interrupted with external_skill_gc activation
                    general_conversation_dialogs.append({'dialog': current_gc_dialog,
                                                         'fielddate': record['fielddate'],
                                                         'feedback': '_no_feedback_'})
                    current_gc_dialog = [req]
                    current_dialog_type = 'external_skill_gc'
                elif request_response_type is None:
                    # if general conversation is interrupted with anything else
                    general_conversation_dialogs.append({'dialog': current_gc_dialog,
                                                         'fielddate': record['fielddate'],
                                                         'feedback': '_no_feedback_'})
                    current_gc_dialog = []
                    current_dialog_type = None
                # end if - no "else" since there are no other possibilities
            # end if
        # end for

        # in case dialog ended with entire session:
        if current_gc_dialog:
            if current_dialog_type == 'external_skill_gc':
                external_skill_gc_dialogs.append({'dialog': current_gc_dialog,
                                                  'fielddate': record['fielddate'],
                                                  'feedback': '_no_feedback_'})
            elif current_dialog_type == 'general_conversation':
                general_conversation_dialogs.append({'dialog': current_gc_dialog,
                                                     'fielddate': record['fielddate'],
                                                     'feedback': '_no_feedback_'})

        # now all we need to do is yield all saved dialogs to their corresponding stream
        # first save the fields which we want to use as slices, as it will be reused
        data_to_multiply = {key: (record.get(key) if record.get(key) is not None else '_unknown_')
                            for key in FIELDS_AS_SLICES}

        data_to_multiply['gc_type'] = 'general_conversation'
        for general_conversation in general_conversation_dialogs:
            data_to_multiply['feedback'] = general_conversation.get('feedback', '_no_feedback_')

            # first individual request in dialog is not relevant for statistics
            for req in general_conversation['dialog'][1:]:
                # yield individual record stats
                multiply_records(get_individual_record_stats(req, record['fielddate']),
                                 data_to_multiply,
                                 individual_record_stats_stream)

            # yield dialog stats
            # construct fixed_data dictionary all the stats + 'fielddate'
            dialog_stats = get_dialog_stats(general_conversation.get('dialog'))
            dialog_stats['fielddate'] = general_conversation.get('fielddate')

            multiply_records(dialog_stats,
                             data_to_multiply,
                             dialog_stats_stream)

        # same thing for external_skill_gc_dialogs
        data_to_multiply['gc_type'] = 'external_skill_gc'
        for external_skill_gc in external_skill_gc_dialogs:
            data_to_multiply['feedback'] = external_skill_gc.get('feedback', '_no_feedback_')

            # first individual request in dialog is not relevant for statistics
            for req in external_skill_gc['dialog'][1:]:
                # yield individual record stats
                multiply_records(get_individual_record_stats(req, record['fielddate']),
                                 data_to_multiply,
                                 individual_record_stats_stream)

            # yield dialog stats
            # construct fixed_data dictionary all the stats + 'fielddate'
            dialog_stats = get_dialog_stats(external_skill_gc.get('dialog'))
            dialog_stats['fielddate'] = external_skill_gc.get('fielddate')

            multiply_records(dialog_stats,
                             data_to_multiply,
                             dialog_stats_stream)

def make_session(groups):
    for key, records in groups:
        session = []
        res = {}
        for record in records:
            record_dict = record.to_dict()
            session.append(record_dict)
            if not res:
                res = {field: record.get(field) for field in FIELDS_AS_SLICES}
                res['fielddate'] = record['fielddate']
        res['session'] = session

        yield Record(**res)


def compute_metrics(date_param, report_config, expboxes_root, intensity):
    if date_param[0] == date_param[1]:
        date_str = date_param[0]
    else:
        date_str = '{' + date_param[0] + '..' + date_param[1] + '}'

    cluster = clusters.Hahn()
    job = cluster.job()

    # super soft back compatibility version with old sessions table
    sessions = job.table(expboxes_root + '/' + date_str) \
        .project(
            'session_id', 'session_sequence', 'fielddate', 'intent', 'query', 'reply', *FIELDS_AS_SLICES,
            delta=ne.custom(lambda other: other.get('delta'), 'other')
        ) \
        .groupby('session_id') \
        .sort('session_sequence') \
        .reduce(make_session)

    dialog_stats, individual_record_stats = sessions.map(split_general_conversation_and_gc, intensity=intensity)

    dialog_stats = dialog_stats.groupby('feedback', 'fielddate', 'gc_type', *FIELDS_AS_SLICES) \
        .aggregate(number_of_dialogs=na.count(),
                   mean_dialog_length=na.mean('dialog_length'),
                   mean_dialog_duration=na.mean('dialog_duration'),
                   mean_dialog_log_length=na.mean('dialog_log_length'),
                   mean_dialog_log_duration=na.mean('dialog_log_duration'))

    individual_record_stats = individual_record_stats.groupby('feedback', 'fielddate', 'gc_type', *FIELDS_AS_SLICES) \
        .aggregate(mean_reply_length=na.mean('reply_length'),
                   mean_query_length=na.mean('query_length'),
                   mean_delta=na.mean('delta'),
                   mean_log_delta=na.mean('log_delta'))

    dialog_stats.join(individual_record_stats, by=['fielddate', 'gc_type', 'feedback'] + FIELDS_AS_SLICES) \
        .filter(nf.not_(nf.equals('version', ''))) \
        .publish(report_config)

    job.run()


def main(start_date, end_date, report_path, expboxes_root='//home/alice/dialog/prepared_logs_expboxes', intensity=None):
    client = make_stat_client()
    date_param = [start_date, end_date]
    report_config = ns.StatfaceReport().path(report_path) \
        .scale('daily') \
        .dimensions(
        ns.Date('fielddate'),
        ns.StringSelector('gc_type'),
        ns.StringSelector('feedback'),
        *[ns.StringSelector(field) for field in FIELDS_AS_SLICES]
    ).measures(
        ns.Number('mean_dialog_length'),
        ns.Number('mean_dialog_duration'),
        ns.Number('mean_dialog_log_length'),
        ns.Number('mean_dialog_log_duration'),
        ns.Number('mean_reply_length'),
        ns.Number('mean_query_length'),
        ns.Number('mean_delta'),
        ns.Number('mean_log_delta'),
        ns.Number('number_of_dialogs')
    ).client(client)

    compute_metrics(date_param, report_config, expboxes_root, intensity)


if __name__ == '__main__':
    call_as_operation(main)
