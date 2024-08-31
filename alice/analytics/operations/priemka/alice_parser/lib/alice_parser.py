# coding: utf-8

from builtins import object
import os
import sys
from functools import partial
import logging

from nile.api.v1 import (
    Record,
    clusters,
    with_hints,
    extended_schema,
    extractors as ne,
    datetime as nd,
    filters as nf,
)
from qb2.api.v1 import (
    extractors as qe,
    filters as qf,
    typing as qt,
)

from alice.analytics.tasks.va_571.basket_configs import get_basket_param
from alice.analytics.operations.priemka.alice_parser.visualize.visualize_request import get_request_visualize_data
from alice.analytics.operations.priemka.alice_parser.utils.hash_utils import get_hash_for_json
from alice.analytics.operations.dialog.sessions.intent_scenario_mapping import get_generic_scenario
from alice.analytics.operations.dialog.sessions.usage_fields import (
    get_music_answer_type_from_analytics_info,
    get_filters_genre,
)
from alice.analytics.utils.auth import choose_credential
from alice.analytics.utils.json_utils import get_path_str
from alice.analytics.utils.yt.basket_common import (
    clean_additional_options,
    get_filtration_level,
    get_common_filters
)
from alice.analytics.utils.yt.extract_asr import get_asr, get_chosen
from alice.analytics.utils.yt.extract_intent import (
    get_vins_response,
    get_product_scenario_name,
    get_mm_scenario,
    get_skill_id,
    get_music_genre,
    get_intent
)
from alice.analytics.operations.priemka.alice_parser.utils.errors import (
    get_preliminary_downloader_result,
)
from alice.analytics.operations.priemka.alice_parser.utils.queries_utils import (
    prepare_toloka_queries,
)

from .yql_custom_funcs import get_location_by_coordinates, get_location_by_region_id, get_location_by_client_ip
from .utils import (
    select_location,
    prepare_voice_url,
    get_voice_url_from_mds_key,
    split_sessions_by_duplicates,
    parse_downloader_client_time,
    extract_key_from_mds_filename,
    get_childness,
)
from .make_session import (
    make_deep_session,
    FULL_SESSION_SCHEMA,
    LONG_SESSION_SCHEMA,
    DSAT_SESSION_SCHEMA,
    TOLOKA_TASKS_EXCLUDE_FIELDS,
    PULSAR_RESULTS_EXCLUDE_FIELDS,
)

from .prepare_for_render import (
    get_screenshot_hashable,
    get_response_dialog,
)

from .hashing import (
    get_hashable
)


@with_hints(output_schema=extended_schema(
    action=qt.Optional[qt.Json],
    hashable=qt.Optional[qt.Json],
    state=qt.Optional[qt.Json],
    answer_standard=qt.Optional[qt.String],
    music_entity=qt.Optional[qt.Json],
    query_eosp=qt.Optional[qt.String],
))
def visualize_data(records, is_quasar, need_voice_url, is_dsat, always_voice_text):
    for record in records:
        r = record.to_dict()

        # иногда на каждый запрос формируется несколько "заданий в толоке" с одинаковым req_id, но разным hashsum
        toloka_queries = list(prepare_toloka_queries(r['query'], split_by_eosp_tag=not is_dsat))
        for idx, _query in enumerate(toloka_queries):
            r['_query'] = _query
            data = get_request_visualize_data(
                r,
                is_quasar=is_quasar,
                only_smart_speakers=r.get('only_smart_speakers', False),
                need_voice_url=need_voice_url,
                is_dsat=is_dsat,
                always_voice_text=always_voice_text
            )

            fields = dict(
                action=data.get('action'),
                hashable=get_hashable(r, data.get('action'), is_quasar=is_quasar),
                state=data.get('state'),
                answer_standard=data.get('answer_standard'),
                music_entity=data.get('music_entity'),
            )

            # запрос "до паузы" сохраняем в отдельную колонку
            if len(toloka_queries) == 2:
                fields['query_eosp'] = toloka_queries[0]

            yield Record(record, **fields)


def make_session_reducer(groups, is_quasar, need_voice_url, is_dsat, merge_tech_queries):
    """
    Редьюсер, который из массива визуализированных запросов формирует одну строку со всей сессией
    Т.е. group by session_id

    Помимо того, что все запросы схлопываются в один запрос (сессию), есть дополнительная логика:
    * в корзинке и в результате прокачки, в сессии запросов может быть больше прокачано,
        чем требуется по асессорской разметке на глубину контекстов. В таком случае в толоку/пульсар уйдёт
        только требуемое число запросов (по числу context_len у последнего запроса в сесии)
    * На предыдущем шаге (visualize) последние запросы могут дублироваться (на случай <EOSp> тега в транскрибации в колонке text)
        Тогда нужно корректно сформировать 2 сессии с учётом последнего дубликата и разметки на context_len
        см пример в https://yql.yandex-team.ru/Operations/YaIiiNJwbOR642lY1yvGhMuRjcIdpsZnkK5t0lk2EVg=
    :param stream groups:
    :param bool is_quasar:
    :param bool need_voice_url:
    :param bool is_dsat:
    :param bool merge_tech_queries:
    """
    for key_session_id, records in groups:
        records_list = [x.to_dict() for x in records]
        context_len = records_list[-1].get('context_len')

        if context_len is None:
            # по-умолчанию берём все запросы из сессии
            records_count = 0
        else:
            # если задано число контекстов (асессорская разметка), то к контекстам добавляем:
            # * сам последний запрос
            # * ровно один дубликат для последнего запроса в случае <EOSp> тега
            records_count = context_len + 1

            if records_list[-1].get('text') and '<EOSp>' in records_list[-1].get('text'):
                records_count += 1

        for session_records_list in split_sessions_by_duplicates(records_list[-records_count:]):
            result = make_deep_session(session_records_list, is_quasar, need_voice_url, is_dsat, merge_tech_queries)
            yield Record.from_dict(result)


class AliceParser(object):
    def __init__(self,
                 basket_table,
                 downloader_result_table,
                 result_table,
                 toloka_tasks_table,
                 url_table=None,
                 cluster='hahn',
                 yt_proxy=None,
                 yql_proxy=None,
                 pool=None,
                 token=None,
                 override_basket_params=None,
                 udf_path=None,
                 store_checkpoints=False,
                 mode='quasar',
                 tables_ttl=14,
                 start_date=None,
                 end_date=None,
                 merge_tech_queries=False,
                 need_voice_url=False,
                 is_dsat=False,
                 always_voice_text=False,
                 ):
        self.mode = mode
        self.basket_table = basket_table
        self.downloader_result_table = downloader_result_table
        self.result_table = result_table
        self.toloka_tasks_table = toloka_tasks_table
        self.url_table = url_table
        self.cluster = cluster
        self.yt_proxy = yt_proxy
        self.yql_proxy = yql_proxy
        self.pool = pool
        self.token = choose_credential(token, 'YT_TOKEN', '~/.yt/token')
        self.override_basket_params = override_basket_params
        self.udf_path = udf_path
        self.store_checkpoints = store_checkpoints
        self.tables_ttl = tables_ttl
        self.start_date = start_date
        self.end_date = end_date
        self.merge_tech_queries = merge_tech_queries
        self.need_voice_url = need_voice_url
        self.is_dsat = is_dsat
        self.always_voice_text = always_voice_text

    def _get_yql_python_udf_path(self):
        filename = sys.executable
        udf_path_list = [os.path.dirname(filename)] + self.udf_path.split('/')

        return os.path.join(*udf_path_list)

    def _get_ttl(self):
        return nd.timedelta(days=self.tables_ttl)

    def init_nile(self):
        env_templates = {
            'title': 'AliceParser {}'.format(self.mode),
            'checkpoints_root': os.path.dirname(self.result_table),
        }

        if self.start_date and self.end_date:
            date_str = '{' + self.start_date + '..' + self.end_date + '}'
            env_templates['date'] = date_str

        cluster_params = dict(
            yql_token=self.token,
            yql_token_for={'yt'},
            pool=self.pool,
        )

        if self.yt_proxy:
            cluster_params['proxy'] = self.yt_proxy
        else:
            cluster_params['proxy'] = '{}.yt.yandex.net'.format(self.cluster)

        if self.yql_proxy:
            cluster_params['yql_proxy'] = self.yql_proxy
        else:
            cluster_params['yql_proxy'] = 'yql.yandex.net'
            cluster_params['yql_web_proxy'] = 'yql.yandex-team.ru'

        envs = {}
        if os.environ.get('LANGUAGE'):
            envs['LANGUAGE'] = os.environ['LANGUAGE']
        logging.info('jobs os enviroment variables: {}'.format(envs))

        cluster = clusters.yql.YQL(**cluster_params).env(
            templates=env_templates,
            yt_spec_defaults=dict(
                pool_trees=["physical"],
                tentative_pool_trees=["cloud"]
            ),
            compression_level=dict(
                final_tables='lightest',  # TODO: heaviest
                tmp_tables='lightest',
            ),
            yql_python_udf_path=self._get_yql_python_udf_path(),
            operations_environment=envs,
        )
        self.job = cluster.job()
        return self

    def run_nile(self):
        transaction = os.getenv('YT_TRANSACTION')
        if transaction:
            with self.job.driver.transaction(transaction):
                return self.job.run(store_checkpoints=self.store_checkpoints)
        else:
            return self.job.run(store_checkpoints=self.store_checkpoints)

    def run(self):
        self.init_nile()

        if self.mode == 'quasar':
            self.parse_quasar()
        elif self.mode == 'general':
            self.parse_general()
        elif self.mode == 'prepare_for_render':
            self.prepare_for_render()
        elif self.mode == 'logs':
            self.parse_logs()
        elif self.mode == 'fat_basket':
            self.parse_fat_basket()

        return self.run_nile().yql_operation_id

    def get_basket_table_stream(self):
        basket_weak_schema = {
            'location': qt.Optional[qt.Json],
            'additional_options': qt.Optional[qt.Json],
            'toloka_extra_state': qt.Optional[qt.Json],
            'basket': qt.Optional[qt.String],
            'exact_location': qt.Optional[qt.String],
            'toloka_intent': qt.Optional[qt.String],
            'voice_url': qt.Optional[qt.String],
            'session_sequence': qt.Optional[qt.Int64],
            'context_len': qt.Optional[qt.Int64],
            'device_state': qt.Optional[qt.Json],
            'is_negative_query': qt.Optional[qt.Int32],
        }

        return self.job.table(self.basket_table, weak_schema=basket_weak_schema)

    def get_prepared_table_stream(self):
        """
        Берет табличку формата prepared_logs_expboxes
        Для важных колонок определяются типы
        :return:
        """
        prepared_weak_schema = {
            'session_id': qt.Optional[qt.String],
            'client_time': qt.Optional[qt.Int64],
            'client_tz': qt.Optional[qt.String],
            'req_id': qt.Optional[qt.String],
            'session_sequence': qt.Optional[qt.Int64],
            'context_len': qt.Optional[qt.Int64],
            'query': qt.Optional[qt.String],
            'reply': qt.Optional[qt.String],
            'voice_text': qt.Optional[qt.String],
            'analytics_info': qt.Optional[qt.Json],
            'intent': qt.Optional[qt.String],
            'generic_scenario': qt.Optional[qt.String],
            'input_type': qt.Optional[qt.String],
            'uuid': qt.Optional[qt.String],
        }

        return self.job.table(self.basket_table, weak_schema=prepared_weak_schema)

    def prepare_prepared_logs(self, stream):
        return stream.project(
            ne.all(exclude=['location', 'analytics_info']),
            basket_location='location',
            new_format=ne.const(True).with_type(qt.Bool),
            parse_iot=ne.const(True).with_type(qt.Bool),
            exact_location=ne.const('').with_type(qt.String),
        ) \
            .filter(*get_common_filters())

    def prepare_basket(self, stream, override_basket_params=None):
        return stream.project(
            'text',
            'session_id',
            'session_sequence',
            'toloka_intent',
            'additional_options',
            'toloka_extra_state',
            'exact_location',
            'basket',
            'context_len',
            'is_negative_query',

            'device_state',  # TODO: брать из результатов прокачки

            req_id='request_id',
            app='app_preset',
            basket_location='location',
            query='text',
            voice_url=ne.custom(prepare_voice_url, 'voice_url').with_type(qt.Optional[qt.String]),
            region_id=ne.custom(lambda x: x.get('bass_options', {}).get('region_id') if x is not None else None,
                                'additional_options')
                .with_type(qt.Optional[qt.Int32]),
            client_ip=ne.custom(lambda x: x.get('bass_options', {}).get('client_ip') if x is not None else None,
                                'additional_options')
                .with_type(qt.Optional[qt.String]),
            filtration_level=ne.custom(get_filtration_level, 'device_state', 'additional_options')
                .with_type(qt.Optional[qt.String]),
            new_format=ne.const(True).with_type(qt.Bool),
            parse_iot=ne.const(False).with_type(qt.Bool),
            only_smart_speakers=ne.custom(
                lambda x: False if x == 'input_basket' else get_basket_param('only_smart_speakers', basket_alias=x, override_basket_params=override_basket_params),
                'basket').with_type(qt.Bool),
        )

    def select_location(self, stream):
        return stream \
            .project(
                ne.all(),
                qe.yql_custom('location_by_coordinates', get_location_by_coordinates(), 'basket_location'),
                qe.yql_custom('location_by_region_id', get_location_by_region_id(), 'region_id'),
                qe.yql_custom('location_by_client_ip', get_location_by_client_ip(), 'client_ip'),
                location=ne.custom(
                    select_location,
                    'app', 'generic_scenario',
                    'exact_location', 'location_by_coordinates', 'location_by_region_id', 'location_by_client_ip'
                ).with_type(qt.Optional[qt.String]),
            )

    def get_downloader_result_table(self):
        return self.job.table(self.downloader_result_table)

    def get_voice_table(self):
        return self.job.table("//home/voice-speechbase/uniproxy/logs_unified_qloud/@date", ignore_missing=True)

    def get_vins_table(self):
        return self.job.table("//home/voice/vins/logs/dialogs/@date", ignore_missing=True)

    def get_url_table_stream(self):
        if self.url_table is None:
            return None

        url_weak_schema = {
            'downloadUrl': qt.Optional[qt.String],
            'initialFileName': qt.Optional[qt.String],
            'mdsFileName': qt.Optional[qt.String],
        }

        return self.job.table(self.url_table, weak_schema=url_weak_schema)

    def prepare_downloader_result(self, stream):
        return stream \
            .project(
                ne.all(),
                vins_response=ne.custom(lambda x: get_vins_response(x, False) if x is not None else None, 'VinsResponse')
                    .with_type(qt.Optional[qt.Json]),
                analytics_info=ne.custom(lambda x: get_path_str(x, 'megamind_analytics_info'), 'vins_response')
                    .with_type(qt.Optional[qt.Json]),
            ) \
            .filter(
                # для валидного запроса или определён analytics_info (а значит и vins_response), или пустой VinsResponse
                qf.or_(
                    qf.defined('analytics_info'),
                    qf.not_(qf.defined('VinsResponse'))
                )
            ) \
            .project(
                'RequestId',
                'vins_response',
                'analytics_info',
                intent=ne.custom(lambda v: get_intent(v, False) or 'EMPTY', 'VinsResponse')
                    .with_type(qt.Optional[qt.String]),
                mm_scenario=ne.custom(lambda v: get_mm_scenario(v, False), 'VinsResponse')
                    .with_type(qt.Optional[qt.String]),
                product_scenario=ne.custom(lambda v: get_product_scenario_name(v, False), 'VinsResponse')
                    .with_type(qt.Optional[qt.String]),
                childness=ne.custom(get_childness, 'BioResponse').with_type(qt.Optional[qt.String]),
                setrace_url='SetraceUrl',
                tz='Timezone',
                ts=ne.custom(parse_downloader_client_time, 'ClientTime').with_type(qt.Optional[qt.Int64]),
                asr_text=ne.custom(get_asr, 'VinsResponse', 'AsrResponses').with_type(qt.String),
                chosen_text=ne.custom(get_chosen, 'VinsResponse').with_type(qt.String),
                music_genre=ne.custom(lambda analytics_info: get_music_genre(analytics_info), 'analytics_info')
                    .with_type(qt.Optional[qt.String]),
                filters_genre=ne.custom(lambda analytics_info: get_filters_genre(analytics_info), 'analytics_info')
                    .with_type(qt.Optional[qt.String]),
                music_answer_type=ne.custom(lambda analytics_info: get_music_answer_type_from_analytics_info(analytics_info), 'analytics_info')
                    .with_type(qt.Optional[qt.String]),
                skill_id=ne.custom(lambda analytics_info: get_skill_id(analytics_info), "analytics_info")
                    .with_type(qt.Optional[qt.String]),
                is_trash=ne.const(False).with_type(qt.Optional[qt.Bool]),
                generic_scenario=ne.custom(get_generic_scenario,
                                           'intent',
                                           'mm_scenario',
                                           'product_scenario',
                                           'music_genre',
                                           "skill_id",
                                           'is_trash',
                                           'music_answer_type',
                                           'filters_genre'
                                           ).with_type(
                    qt.Optional[qt.String]),
                reply=ne.custom(lambda x: get_path_str(x, 'response.cards.0.text'), 'vins_response')
                    .with_type(qt.Optional[qt.String]),
                voice_text=ne.custom(lambda x: get_path_str(x, 'voice_response.output_speech.text'), 'vins_response')
                    .with_type(qt.Optional[qt.String]),
                directives=ne.custom(lambda x: get_path_str(x, 'response.directives', []), 'vins_response')
                    .with_type(qt.Optional[qt.Json]),
                meta=ne.custom(lambda x: get_path_str(x, 'response.meta', []), 'vins_response')
                    .with_type(qt.Optional[qt.Json]),
            ) \
            .project(ne.all(exclude=['music_genre', "skill_id", 'filters_genre', 'music_answer_type', 'is_trash']))

    def prepare_voice_table(self, stream):
        return stream \
            .filter(qf.defined('mds_key')) \
            .filter(qf.defined('vins_request_id')) \
            .filter(qf.defined('uuid')) \
            .project(ne.all(exclude='uuid'),
                     uuid=ne.custom(lambda uuid: 'uu/' + uuid, 'uuid').with_type(qt.Optional[qt.String])) \
            .filter(*get_common_filters()) \
            .project(
                'mds_key',
                voice_url=ne.custom(get_voice_url_from_mds_key, 'mds_key').with_type(qt.Optional[qt.String]),
                req_id='vins_request_id'
            )

    def prepare_vins_table(self, stream):
        return stream \
            .filter(*get_common_filters()) \
            .project(
                'form_name',
                'response',
                'analytics_info',
                req_id=ne.custom(lambda request: request.get('request_id'), 'request').with_type(qt.Optional[qt.String]),
                device_state=ne.custom(lambda request: request.get('device_state') or {}, 'request').with_type(
                    qt.Optional[qt.Json]),
                request_additional_options=ne.custom(lambda request: request.get('additional_options'),
                                                     'request').with_type(qt.Optional[qt.Json]).hide(),
                additional_options=ne.custom(clean_additional_options, 'request_additional_options').with_type(
                    qt.Optional[qt.Json]),
                filtration_level=ne.custom(get_filtration_level, 'device_state', 'additional_options').with_type(
                    qt.Optional[qt.String]),
                directives=ne.custom(lambda x: get_path_str(x, 'directives', []), 'response').with_type(
                    qt.Optional[qt.Json]),
            ) \
            .filter(nf.not_(nf.equals('req_id', None))) \
            .project(
                ne.all(),
                region_id=ne.custom(lambda x: x.get('bass_options', {}).get('region_id') if x is not None else None,
                                    'additional_options').with_type(qt.Optional[qt.Int32]),
                client_ip=ne.custom(lambda x: x.get('bass_options', {}).get('client_ip') if x is not None else None,
                                    'additional_options').with_type(qt.Optional[qt.String]),
            )

    def join_basket_and_downloader_result(self, basket, downloader_result):
        return basket.join(
            downloader_result,
            type='left',
            by_left='req_id',
            by_right='RequestId'
        )

    def join_prepared_and_voice(self, prepared, voice):
        return prepared.join(  # voice_requests
            voice,
            type='left',
            by='req_id',
            assume_small_left=True
        ) \
            .unique('req_id')

    def join_prepared_and_vins(self, voice_requests, prepared, vins_logs):
        return self.job.concat(
            voice_requests,
            prepared.filter(nf.or_(nf.equals('input_type', 'text'), nf.equals('input_type', 'tech'),
                                   nf.equals('input_type', 'scenario')))
        ) \
            .join(vins_logs, by='req_id', assume_small_left=True) \
            .unique('req_id')

    def prepare_merged_basket_downloader_results(self, stream):
        return stream.project(
            ne.all(['RequestId', 'filtration_level']),
            result=ne.custom(
                get_preliminary_downloader_result,
                'text', 'RequestId', 'vins_response', 'generic_scenario'
            ).with_type(qt.Optional[qt.String]),
            filtration_level=ne.custom(get_filtration_level, 'device_state', 'additional_options', 'childness')
                .with_type(qt.Optional[qt.String]),
        )

    def join_basket_download(self):
        basket_table = self.get_basket_table_stream()
        basket = self.prepare_basket(basket_table, self.override_basket_params)

        downloader_result_table = self.get_downloader_result_table()
        downloader_result = self.prepare_downloader_result(downloader_result_table)

        result = self.join_basket_and_downloader_result(basket, downloader_result)

        result = self.prepare_merged_basket_downloader_results(result)
        result = self.select_location(result)

        return result

    def join_data_from_logs(self, need_join_voice_logs=False):
        prepared_table = self.get_prepared_table_stream()
        prepared_table = self.prepare_prepared_logs(prepared_table)

        if need_join_voice_logs:
            # оставляем join с logs_unified_qloud для экстренных случаев
            voice_table = self.get_voice_table()
            voice_table = self.prepare_voice_table(voice_table)
            requests = self.join_prepared_and_voice(
                prepared_table.filter(nf.equals('input_type', 'voice')),
                voice_table
            )
        else:
            # по-умолчанию в режиме logs, задания отрисовываются без voice_url и голосовых логов
            requests = prepared_table

        vins_table = self.get_vins_table()
        vins_table = self.prepare_vins_table(vins_table)

        joined_data = self.join_prepared_and_vins(requests, prepared_table, vins_table)

        result = self.select_location(joined_data)

        return result

    def visualize(self, stream, is_quasar=True, need_voice_url=False, is_dsat=False, always_voice_text=False):
        return stream \
            .checkpoint('before_visualize') \
            .map(partial(
                visualize_data,
                is_quasar=is_quasar,
                need_voice_url=need_voice_url,
                is_dsat=is_dsat,
                always_voice_text=always_voice_text
            )) \
            .checkpoint('after_visualize') \
            .project(ne.all())

    def make_session(self, stream, is_quasar=True, need_voice_url=False, is_dsat=False, is_long=False):
        sessions_schema = DSAT_SESSION_SCHEMA if is_dsat else LONG_SESSION_SCHEMA if is_long else FULL_SESSION_SCHEMA

        return stream \
            .checkpoint('before_sessions') \
            .groupby('session_id') \
            .sort('session_sequence') \
            .reduce(
                with_hints(output_schema=sessions_schema)(
                    partial(
                        make_session_reducer,
                        is_quasar=is_quasar,
                        need_voice_url=need_voice_url,
                        is_dsat=is_dsat,
                        merge_tech_queries=self.merge_tech_queries
                    )
                )
            ) \
            .checkpoint('after_sessions') \
            .project(ne.all())

    def prepare_toloka_tasks(self, stream):
        """
        Формирует задания для Толоки:
            * не отправляем на оценку те задания, где `result` есть (была какая-либо ошибка при формировании задания)
            * оставляет только необходимые поля для Толоки
        :param stream:
        :return:
        """
        return stream \
            .filter(
                nf.and_(
                    nf.not_(qf.defined('result')),
                    nf.not_(nf.equals('is_negative_query', 1))
                )
            ) \
            .project(ne.all(exclude=TOLOKA_TASKS_EXCLUDE_FIELDS))

    def prepare_pulsar_results(self, stream, is_quasar=True):
        """
        Формирует табличку с результатами для отображения в Пульсаре
        :param stream:
        :param bool is_quasar:
        :return:
        """
        fields = dict(
            answer=ne.custom(lambda x: get_path_str(x, '-1.answer'), 'session').with_type(qt.Optional[qt.String]),
            action=ne.custom(lambda x: get_path_str(x, '-1.action'), 'session').with_type(qt.Optional[qt.String]),
            query=ne.custom(lambda x: get_path_str(x, '-1.query'), 'session').with_type(qt.Optional[qt.String]),
            generic_scenario_human_readable=ne.custom(lambda x: get_path_str(x, '-1.scenario'), 'session').with_type(
                qt.Optional[qt.String]),
        )
        if is_quasar is False:
            fields['screenshot_url'] = ne.custom(lambda x: get_path_str(x, '-1.url'), 'session').with_type(
                qt.Optional[qt.String])

        return stream \
            .project(
                ne.all(exclude=PULSAR_RESULTS_EXCLUDE_FIELDS),
                **fields
            ) \
            .groupby('req_id') \
            .top(1, by='query', mode='max') \
            .sort('req_id')

    def get_screenshots_data(self, stream):
        """
        Подготавливает данные, которые используются для снятия скриншотов ПП
        Возвращает nile-поток с колонками:
            * req_id
            * dialog - данные с div'ными карточками
            * screenshot_hashable - объект с данными, по которым считается hashsum
            * screenshot_hashsum — hashsum от данных
        """
        return stream \
            .checkpoint('before_screenshot_data') \
            .project(
                'req_id',
                dialog=ne.custom(get_response_dialog, 'text', 'vins_response').with_type(qt.Optional[qt.Json]),
                screenshot_hashable=ne.custom(get_screenshot_hashable, 'text', 'vins_response').with_type(
                    qt.Optional[qt.Json]),
                screenshot_hashsum=ne.custom(get_hash_for_json, 'screenshot_hashable').with_type(qt.Optional[qt.String]),
            ) \
            .filter(qf.defined('dialog')) \
            .unique('screenshot_hashsum')

    def join_basket_screenshots(self, joined_results, url_table=None):
        """
        К результатам прокачки-корзинки добавляет скриншоты
        :param Stream joined_results:
        :param Optional[Stream] url_table:
        :return:
        """
        joined_results_prepared = joined_results \
            .project(
                ne.all(),
                screenshot_hashable=ne.custom(get_screenshot_hashable, 'text', 'vins_response')
                    .with_type(qt.Optional[qt.Json]),
                screenshot_hashsum=ne.custom(get_hash_for_json, 'screenshot_hashable').with_type(qt.Optional[qt.String])
            )

        if url_table is None:
            return joined_results_prepared.project(
                ne.all(),
                screenshot_absent=ne.const(True).with_type(qt.Optional[qt.Bool])
            )

        url_table_prepared = url_table \
            .project(
                screenshot_url='downloadUrl',
                screenshot_hashsum=ne.custom(extract_key_from_mds_filename, 'initialFileName')
                    .with_type(qt.Optional[qt.String]),
            )

        return joined_results_prepared \
            .join(
                url_table_prepared,
                by='screenshot_hashsum',
                type='left'
            )

    def prepare_results(self, input_stream, is_quasar=True, is_long=False, need_voice_url=False, is_dsat=False, always_voice_text=False):
        """
        Основной метод парсера:
            * визуализирует действия Алисы
            * собирает запросы и контексты в сессию по session_id
            * формирует задания в Толоку и результаты Пульсара
        :param input_stream:
        :param bool is_quasar:
        """
        visualized = self.visualize(
            input_stream,
            is_quasar=is_quasar,
            need_voice_url=need_voice_url,
            is_dsat=is_dsat,
            always_voice_text=always_voice_text
        )

        full_sessions = self.make_session(visualized, is_quasar=is_quasar, is_long=is_long, need_voice_url=need_voice_url, is_dsat=is_dsat)

        toloka_tasks = self.prepare_toloka_tasks(full_sessions)
        toloka_tasks.put(self.toloka_tasks_table, ttl=self._get_ttl())

        pulsar_results = self.prepare_pulsar_results(full_sessions, is_quasar=is_quasar)
        pulsar_results.put(self.result_table, ttl=self._get_ttl())

        return pulsar_results

    def parse_quasar(self):
        """
        Подготовка заданий в Толоку и результатов в Пульсаре для колоночных поверхностей
        """
        joined_basket_downaloder_results = self.join_basket_download()
        self.prepare_results(
            joined_basket_downaloder_results,
            always_voice_text=self.always_voice_text,
            need_voice_url=self.need_voice_url,
            is_dsat=self.is_dsat,
        )

    def parse_general(self):
        """
        Подготовка заданий в Толоку и результатов в Пульсаре для поверхностей со скриншотами ПП/Я.Бро
        """
        joined_basket_downloader_results = self.join_basket_download()

        joined_screenshots = self.join_basket_screenshots(joined_basket_downloader_results, self.get_url_table_stream())

        self.prepare_results(
            joined_screenshots,
            is_quasar=False,
            always_voice_text=self.always_voice_text,
            need_voice_url=self.need_voice_url,
            is_dsat=self.is_dsat,
        )

    def prepare_for_render(self):
        """
        Формирует табличку с данными `results_table`, по данным из которой будут сделаны скриншоты ПП
        """
        join_basket = self.join_basket_download()
        for_render = self.get_screenshots_data(join_basket)
        for_render.put(self.result_table, ttl=self._get_ttl())

    def parse_logs(self):
        """
        Подготовка заданий в Толоку и результатов в Пульсаре для логов
        """
        joined_data_from_logs = self.join_data_from_logs(need_join_voice_logs=self.need_voice_url)
        self.prepare_results(
            joined_data_from_logs,
            is_long=True,
            need_voice_url=self.need_voice_url,
            is_dsat=self.is_dsat,
            always_voice_text=True,
        )

    def parse_fat_basket(self):
        """
        Подготовка заданий в Толоку и результатов в Пульсаре для логов на основании логов с дополнительными полями
        (на вход подаётся аналог результата join_data_from_logs()
        """
        fat_basket = self.job.table(self.basket_table) \
            .project(
                ne.all(exclude=['location']),
                additional_options=ne.custom(clean_additional_options, 'request_additional_options').with_type(
                    qt.Optional[qt.Json]),
                filtration_level=ne.custom(get_filtration_level, 'device_state', 'additional_options').with_type(
                    qt.Optional[qt.String]),
                region_id=ne.custom(lambda x: x.get('bass_options', {}).get('region_id') if x is not None else None,
                                    'additional_options').with_type(qt.Optional[qt.Int32]),
                client_ip=ne.custom(lambda x: x.get('bass_options', {}).get('client_ip') if x is not None else None,
                                    'additional_options').with_type(qt.Optional[qt.String]),
                basket_location='location',
                exact_location=ne.const('').with_type(qt.String),
            )

        fat_basket = self.select_location(fat_basket)

        self.prepare_results(
            fat_basket,
            is_long=True,
            need_voice_url=self.need_voice_url,
            is_dsat=self.is_dsat,
            always_voice_text=True
        )
