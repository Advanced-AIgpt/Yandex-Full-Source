# -*- coding: utf-8 -*-

# TV ANDROID DSAT
# designed and powered by nik-fedorov

import os

from utils.nirvana.op_caller import call_as_operation

from nile.api.v1 import clusters

from dsat_processing import (
    load_tvandroid_sessions_data,
    add_voice_requests_from_prepared,
    add_view_time_from_strm,
    add_titles_and_ids_from_cards_and_carousels_info,
    add_action_type_column,
    add_x_and_y_for_cards_events,
)

from reducers import main_reducer


def main(**kwargs):
    if kwargs['end_date'] < kwargs['start_date']:
        raise RuntimeError('end_date is earlier than start_date')

    cluster = clusters.Hahn()
    job = cluster.job()

    base_dir = os.path.dirname(os.path.realpath(__file__))
    job = job.env(
        files=[
            os.path.join(base_dir, 'my_utils.py'),
            os.path.join(base_dir, 'reducers.py'),
            os.path.join(base_dir, 'constants.py'),
            os.path.join(base_dir, 'dsat_processing.py'),
        ]
    )

    tvandroid_sessions = load_tvandroid_sessions_data(job, **kwargs)
    # tvandroid_sessions = add_voice_requests_from_prepared(job, tvandroid_sessions, **kwargs)
    tvandroid_sessions = add_view_time_from_strm(job, tvandroid_sessions, **kwargs)
    tvandroid_sessions = add_titles_and_ids_from_cards_and_carousels_info(job, tvandroid_sessions)
    tvandroid_sessions = add_action_type_column(tvandroid_sessions)
    tvandroid_sessions = add_x_and_y_for_cards_events(tvandroid_sessions)

    dsat = tvandroid_sessions.groupby(
        'device_id', 'session_id'
    ).sort(
        'timestamp_ms', 'y', 'x'
    ).reduce(
        main_reducer
    ).put(
        kwargs['output_table']
    )

    job.run()

    return {'output_path': kwargs['output_table']}


if __name__ == '__main__':
    call_as_operation(main)
