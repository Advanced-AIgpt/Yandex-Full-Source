#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from datetime import timedelta

from nile.api.v1 import (
    extractors as ne,
    datetime as nd,
    filters as nf,
)

from qb2.api.v1 import (
    typing as qt,
    filters as qf,
    extractors as qe,
)


from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from utils.common import get_dict_path, set_last_table_on_finish

from recommendations_utils import (
    get_content_id,
    get_content_type,
    get_place,
    get_content_card_position,
    get_carousel_position,
    get_parent_id,
    get_carousel_title
)


TVANDROID_SESSIONS_PATH = '//home/smarttv/logs/tvandroid_sessions/1d/'
CONTENT_INFO_PATH = '//home/sda/cubes/tv/recommendations/sources/content_info'
CAROUSELS_INFO_PATH = '//home/sda/cubes/tv/recommendations/sources/carousel_data_clean'
RECOMMENDATIONS_PATH = '//home/sda/cubes/tv/recommendations/logs'

UPDATE_PERIOD = 7

NOT_RECOMMENDATION_CONTENT_TYPES = frozenset(['channel', 'source', 'expand-carousel', 'web',
                          'embedded_carousel', 'embedded_category', 'entity',
                          'collection', 'association', 'search', 'app'])

PLACES = frozenset(['main', 'movie', 'series', 'kids',
                    'blogger', 'special_event', 'show_more_items'])

HH_PARENT_ID_YQL_UDF = '''
    YQL::Udf(
        AsAtom("Protobuf.TryParse"),
        Void(), Void(),
        AsAtom(@@{
            "name": "NVideoRecom.TCarouselId",
            "lists": {"optional": false},
            "meta": "H4sICI3Fel8AAzEA49rByCWYnFiUX1qcmhOfmaJXUJRfki/E7ReWmZKaH5SanJ+rNJWRizvEGarGM0VIhIvVObEkNV2CUYFRgzMIwhGS4GIPSi0sTS0ukWACi8O4IPUhiemeKRLMEPVgjpASF49zYl5waVJxclFmUqoEC1CSIwhFTEiGi9Oz2D8tLSczL1WCFawAIQAAgJxztrsAAAA="
        }@@)
    )(String::Base64Decode($p0)).Categ
'''


def make_job(job, date, days_update_period=UPDATE_PERIOD, recommendations_cube_dir=RECOMMENDATIONS_PATH, tvandroid_sessions_dir=TVANDROID_SESSIONS_PATH, content_info_path=CONTENT_INFO_PATH, carousels_info_path=CAROUSELS_INFO_PATH):
    base_dir = os.path.dirname(os.path.realpath(__file__))

    update_period_start_date = nd.next_day(date, offset=-days_update_period, scale='daily')
    range_date = "{{{start_date}..{end_date}}}".format(start_date=update_period_start_date, end_date=date)


    content_info = job.table(
        content_info_path
    ).label('content_info').filter(
        qf.defined('id')
    ).project(
        ne.all(exclude=['real_content_id', 'id']),
        content_id='id'
    )

    carousels_info = job.table(
        carousels_info_path
    ).label('carousels_info').filter(
        qf.defined('hh_tag')
    ).project(
        hh_parent_id='hh_tag',
        carousel_title='title'
    )

    recommendations_cube_old = job.table(
        os.path.join(recommendations_cube_dir, 'last')
    ).label('recommendations_cube_old').filter(
        qf.compare('fielddate', '<', update_period_start_date)
    )

    recommendations_cube = job.table(
        tvandroid_sessions_dir + range_date
    ).label('tvandroid_sessions').filter(
        nf.equals('is_factory_ip', False),
        qf.one_of('event_name', ['card_click', 'card_show', 'carousel_scroll'])
    ).project(
        'device_id', 'puid', 'session_id', 'manufacturer', 'model',
        'app_version', 'diagonal', 'resolution', 'is_logged_in',
        'board', 'build', 'firmware_version', 'platform', 'event_name',
        'event_timestamp', 'has_plus', 'quasar_device_id', 'has_gift',

        test_buckets='raw_buckets',
        fielddate='event_date',

        content_id=ne.custom(
            get_content_id,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.String]),

        content_type=ne.custom(
            get_content_type,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.String]),

        place=ne.custom(
            get_place,
            'event_value'
        ).with_type(qt.Optional[qt.String]),

        content_card_position=ne.custom(
            get_content_card_position,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.Int64]),

        carousel_position=ne.custom(
            get_carousel_position,
            'event_name', 'event_value'
        ).with_type(qt.Optional[qt.Int64]),

        parent_id=ne.custom(
            get_parent_id,
            'event_value'
        ).with_type(qt.Optional[qt.String]),

        clid1=ne.custom(
            lambda x: get_dict_path(x, ['clid1'], convert_type=str),
            'clids'
        ).with_type(qt.Optional[qt.String]),

        clid100010=ne.custom(
            lambda x: get_dict_path(x, ['clid100010'], convert_type=str),
            'clids'
        ).with_type(qt.Optional[qt.String]),
    ).filter(
        nf.not_(qf.one_of('content_type', NOT_RECOMMENDATION_CONTENT_TYPES)),
        qf.one_of('place', PLACES)
    ).join(
        content_info,
        by='content_id',
        type='left',
        assume_unique_right=True
    )

    parent_ids = recommendations_cube.project(
        'parent_id',

        hh_parent_id=qe.yql_custom(
            'hh_parent_id',
            HH_PARENT_ID_YQL_UDF,
            'parent_id'
        ).with_type(qt.Optional[qt.String]),

    ).unique(
        'parent_id'
    ).join(
        carousels_info,
        by='hh_parent_id',
        type='left',
        assume_unique_right=True
    ).project(
        ne.all(exclude=['hh_parent_id'])
    )

    recommendations_cube = recommendations_cube.join(
        parent_ids,
        by='parent_id',
        type='left',
        assume_unique_right=True
    ).project(
        ne.all(exclude=['carousel_title']),

        carousel_title=ne.custom(
            get_carousel_title,
            'carousel_title', 'parent_id'
        ).with_type(qt.Optional[qt.String])
    )

    recommendations_cube_all = job.concat(
        recommendations_cube,
        recommendations_cube_old
    ).label('tvandroid_sessions').sort(
        'fielddate',
        'event_name',
        'device_id',
        'event_timestamp'
    ).label('output').put(
        os.path.join(recommendations_cube_dir, date),
        ttl=timedelta(days=4)
    )

    return job


@set_last_table_on_finish("date", "recommendation_cube_dir", RECOMMENDATIONS_PATH)
def main(*args, **kwargs):
    cluster = hahn_with_deps(
        pool="smarttv",
        neighbours_for=__file__,
        include_utils=True,
        use_yql=True,
        custom_cluster_params={"yql_token_for": {"yt"}},
    )
    job = cluster.job()
    job = make_job(job, *args, **kwargs)
    job.run()
    return job


if __name__ == "__main__":
    call_as_operation(main)
    # How to run
    # Setup virtualenv:
    # virtualenv ~/env/analytics -p python
    # source ~/env/analytics/bin/activate

    # Install nile, qb2, fire
    # pip install -i https://pypi.yandex-team.ru/simple nile
    # pip install -i https://pypi.yandex-team.ru/simple qb2
    # pip install fire

    # Install analytics as a package
    # cd ~/arc/arcadia/alice/analytics
    # pip install -e .

    # Launch this script with parameters
    # python main.py --date 2021-01-01
