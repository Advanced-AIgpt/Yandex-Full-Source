# -*- coding: utf-8 -*-

from nile.api.v1 import (
    extractors as ne,
    aggregators as na,
    datetime as nd,
)

from qb2.api.v1 import (
    typing as qt,
    filters as qf,
    extractors as qe,
)

from constants import (
    HH_PARENT_ID_YQL_UDF,
    TVANDROID_SESSIONS_PATH,
    STRM_FILTERED_PATH,
    CONTENT_INFO_PATH,
    CAROUSELS_INFO_PATH,
    PREPARED_LOGS_EXPBOXES_PATH,
)

from my_utils import (
    add_view_time_to_native_player_opened,
    get_content_id_for_root_and_real_content_id,
    get_content_id,
    get_parent_id,
    get_event_value_for_voice_request,
    add_carousels_titles,
    add_content_titles,
    add_root_or_real_content_id,
    define_action_type,
    get_timestamp_ms,
    timestamp_to_datetime,
)


def load_tvandroid_sessions_data(job, **kwargs):
    tvandroid_sessions = job.table(
        TVANDROID_SESSIONS_PATH.format(
            start_date=kwargs['start_date'],
            end_date=kwargs['end_date']
        )
    ).filter(
        qf.defined('device_id', 'session_id')
    )

    # prepare data depending on input arguments
    if 'device_ids' in kwargs:
        tvandroid_sessions = tvandroid_sessions.filter(
            qf.one_of('device_id', kwargs['device_ids']),
        )
    elif 'session_ids' in kwargs:
        tvandroid_sessions = tvandroid_sessions.filter(
            qf.one_of('session_id', kwargs['session_ids']),
        )
    elif 'sessions_num' in kwargs:
        # we will choose sessions which are fully contained in [start_date, end_date]
        period_start = nd.next_day(kwargs['start_date'], offset=-1, scale='daily')
        period_end = nd.next_day(kwargs['end_date'], offset=1, scale='daily')

        # prepare input aggregators and filters
        aggregators = kwargs['aggregators'] if 'aggregators' in kwargs else {}
        for key in aggregators:
            aggregators[key] = eval(aggregators[key])

        filters = kwargs['filters'] if 'filters' in kwargs else []
        for i in range(len(filters)):
            filters[i] = eval(filters[i])

        # choose sample which satisfies all input requirements
        sample = job.table(
            TVANDROID_SESSIONS_PATH.format(
                start_date=period_start,
                end_date=period_end
            )
        ).filter(
            qf.defined('device_id', 'session_id')
        ).groupby(
            'session_id'
        ).aggregate(
            first_event_date=na.min('event_date'),
            last_event_date=na.max('event_date'),
            **aggregators
        ).filter(
            qf.compare('first_event_date', '>=', kwargs['start_date']),
            qf.compare('last_event_date', '<=', kwargs['end_date']),
            *filters
        ).project('session_id').unique('session_id').random(count=kwargs['sessions_num'])

        tvandroid_sessions = tvandroid_sessions.join(
            sample,
            by='session_id',
            type='left_semi',
            assume_unique_right=True
        )
    else:
        raise RuntimeError('Not enough arguments in kwargs')

    tvandroid_sessions = tvandroid_sessions.project(
        'device_id', 'session_id', 'event_name', 'event_value', 'event_datetime', 'quasar_device_id',
        'is_logged_in', 'has_plus', 'has_gift',
        # data below will be saved in properties of session_init events
        'board', 'build', 'diagonal', 'geo_id', 'manufacturer', 'model', 'platform', 'resolution',

        timestamp_ms=ne.custom(
            get_timestamp_ms,
            'event_name', 'event_value', 'event_timestamp', 'event_datetime'
        ).with_type(qt.Optional[qt.Int64]),
    )

    return tvandroid_sessions


def add_voice_requests_from_prepared(job, tvandroid_sessions, **kwargs):
    # get ids data for prepared
    ids_mapping = tvandroid_sessions.filter(
        qf.defined('quasar_device_id')
    ).project(
        'device_id', 'session_id', 'quasar_device_id'
    ).unique(
        'session_id', 'quasar_device_id'
    )

    # load prepared data
    prepared = job.table(
        PREPARED_LOGS_EXPBOXES_PATH.format(
            start_date=kwargs['start_date'],
            end_date=kwargs['end_date']
        )
    ).project(
        'generic_scenario', 'music_answer_type', 'music_genre', 'query', 'reply', 'sound_muted',
        'client_time', 'session_id',
        quasar_device_id='device_id'
    ).join(
        ids_mapping,
        by=('session_id', 'quasar_device_id'),
        type='inner',
        assume_unique_right=True
    ).project(
        'device_id', 'session_id',

        event_name=ne.const('voice_request').with_type(qt.Optional[qt.String]),

        event_value=ne.custom(
            get_event_value_for_voice_request,
            'generic_scenario', 'music_answer_type', 'music_genre', 'query', 'reply', 'sound_muted'
        ).with_type(qt.Optional[qt.Yson]),

        event_datetime=ne.custom(timestamp_to_datetime, 'client_time').with_type(qt.Optional[qt.String]),

        timestamp_ms=ne.custom(lambda ts: ts * 1000, 'client_time').with_type(qt.Optional[qt.Int64])
    )

    # add voice requests in tvandroid_sessions
    tvandroid_sessions = job.concat(
        tvandroid_sessions,
        prepared
    )

    return tvandroid_sessions


def add_view_time_from_strm(job, tvandroid_sessions, **kwargs):
    # load strm data
    strm = job.table(
        STRM_FILTERED_PATH.format(
            start_date=kwargs['start_date'],
            end_date=kwargs['end_date']
        )
    ).project(
        'device_id', 'vsid', 'view_time',
    ).filter(
        qf.defined('device_id', 'vsid', 'view_time'),
    ).groupby(
        'device_id', 'vsid'
    ).aggregate(
        view_time=na.sum('view_time')
    )

    # join view_time to tvandroid_sessions
    tvandroid_sessions = tvandroid_sessions.project(
        ne.all(),

        vsid=ne.custom(
            lambda event_name, event_value: event_value['vsid'] if event_name == 'native_player_opened' else None,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.String])
    ).join(
        strm,
        by=('device_id', 'vsid'),
        type='left',
        assume_unique_right=True
    ).project(
        ne.all(exclude=['vsid', 'view_time', 'event_value']),

        event_value=ne.custom(
            add_view_time_to_native_player_opened,
            'event_name', 'event_value', 'vsid', 'view_time'
        ).with_type(qt.Optional[qt.Yson])
    )

    return tvandroid_sessions


def add_titles_and_ids_from_cards_and_carousels_info(job, tvandroid_sessions):
    # load carousels info
    carousels_info = job.table(
        CAROUSELS_INFO_PATH
    ).filter(
        qf.defined('hh_tag')
    ).unique(
        'hh_tag'
    ).project(
        carousel_title='title',
        hh_parent_id='hh_tag'
    )

    # join titles of carousels
    tvandroid_sessions = tvandroid_sessions.project(
        ne.all(),

        parent_id=ne.custom(
            get_parent_id,  # return None if there is no need to change 'carousel_name' in event_value
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.String])
    ).project(
        ne.all(),

        hh_parent_id=qe.yql_custom(
            'hh_parent_id',
            HH_PARENT_ID_YQL_UDF,
            'parent_id'
        ).with_type(qt.Optional[qt.String])
    ).join(  # join carousels titles
        carousels_info,
        by='hh_parent_id',
        type='left'
    ).project(  # remove unwanted columns and add titles in event_value
        ne.all(exclude=['parent_id', 'hh_parent_id', 'carousel_title', 'event_value']),

        event_value=ne.custom(
            add_carousels_titles,
            'event_value', 'carousel_title'
        ).with_type(qt.Optional[qt.Yson])
    )

    # load cards info
    content_info = job.table(CONTENT_INFO_PATH)

    # prepare cards info for titles mapping by id and onto_id
    titles_with_id = content_info.filter(
        qf.defined('id')
    ).unique(
        'id'
    ).project(
        content_id='id', title1='title'
    )
    titles_with_onto_id = content_info.filter(
        qf.defined('onto_id'),
        qf.not_(qf.defined('episodes_root_content_id'))
    ).unique(
        'onto_id'
    ).project(
        content_id='onto_id', title2='title'
    )

    # add titles to tvandroid_sessions
    tvandroid_sessions = tvandroid_sessions.project(
        ne.all(),

        content_id=ne.custom(
            get_content_id,  # return None if there is no need to add 'title' in event_value
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.String])
    ).join(
        titles_with_id,
        by='content_id',
        type='left',
        assume_unique_right=True
    ).join(
        titles_with_onto_id,
        by='content_id',
        type='left',
        assume_unique_right=True
    ).project(
        ne.all(exclude=['event_value', 'content_id', 'title1', 'title2']),

        event_value=ne.custom(
            add_content_titles,
            'event_name', 'event_value', 'title1', 'title2'
        ).with_type(qt.Optional[qt.Yson])
    )

    # prepare cards info for "root or real" ids mapping by id and onto_id
    ids_with_root_and_real_id = content_info.filter(
        qf.defined('id')
    ).unique(
        'id'
    ).project(
        content_id='id',
        root_id1='episodes_root_content_id',
        real_id1='real_content_id'
    )
    onto_ids_with_root_and_real_id = content_info.filter(
        qf.defined('onto_id')
    ).unique(
        'onto_id'
    ).project(
        content_id='onto_id',
        root_id2='episodes_root_content_id',
        real_id2='real_content_id'
    )

    # add root_or_real_content_id
    tvandroid_sessions = tvandroid_sessions.project(
        ne.all(),

        content_id=ne.custom(
            get_content_id_for_root_and_real_content_id,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.String])
    ).join(
        ids_with_root_and_real_id,
        by='content_id',
        type='left',
        assume_unique_right=True
    ).join(
        onto_ids_with_root_and_real_id,
        by='content_id',
        type='left',
        assume_unique_right=True
    ).project(
        ne.all(exclude=['event_value', 'content_id', 'root_id1', 'real_id1', 'root_id2', 'real_id2']),

        event_value=ne.custom(
            add_root_or_real_content_id,
            'event_value', 'root_id1', 'real_id1', 'root_id2', 'real_id2'
        ).with_type(qt.Optional[qt.Yson])
    )

    return tvandroid_sessions


def add_action_type_column(tvandroid_sessions):
    # add action_type column and filter by action_type
    tvandroid_sessions = tvandroid_sessions.project(
        ne.all(),

        action_type=ne.custom(
            define_action_type,
            'event_name'
        ).with_type(qt.Optional[qt.String]),
    ).filter(
        qf.defined('action_type')
    )

    return tvandroid_sessions


def add_x_and_y_for_cards_events(tvandroid_sessions):
    tvandroid_sessions = tvandroid_sessions.project(
        ne.all(),

        x=ne.custom(
            lambda en, ev: ev.get('x', -1) if en in ['card_show', 'card_click'] else -1,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.Int64]),

        y=ne.custom(
            lambda en, ev: ev.get('y', -1) if en in ['card_show', 'card_click'] else -1,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.Int64])
    )

    return tvandroid_sessions
