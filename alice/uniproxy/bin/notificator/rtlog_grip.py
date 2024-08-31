import alice.uniproxy.library.settings as settings
from alice.uniproxy.library.logging import Logger

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalCountersUpdater

import rtlog
import os


def _update_counters():
    stats = rtlog.get_stats()
    counters = GlobalCounter
    counters.RTLOG_ACTIVE_LOGGERS_AMMM.set(stats.active_loggers_count)
    counters.RTLOG_EVENTS_SUMM.set(stats.events_count)
    counters.RTLOG_PENDING_BYTES_AMMM.set(stats.pending_bytes_count)
    counters.RTLOG_WRITTEN_FRAMES_SUMM.set(stats.written_frames_count)
    counters.RTLOG_WRITTEN_BYTES_SUMM.set(stats.written_bytes_count)
    counters.RTLOG_ERRORS_SUMM.set(stats.errors_count)
    counters.RTLOG_SHRINKED_BYTES_SUMM.set(stats.shrinked_bytes_count)


def init():
    rtlog_config = settings.config.get('rtlog', None)
    if not rtlog_config:
        return
    file_name = rtlog_config.get('file_name')
    if not file_name:
        raise RuntimeError('file_name is not set for rtlog config')
    ydb_settings = rtlog_config.get('ydb')
    if ydb_settings:
        ydb_token = os.environ.get('YDB_TOKEN')
        if not ydb_token:
            raise RuntimeError('YDB_TOKEN not defined')
        ydb_settings = dict(ydb_settings)
        ydb_settings['auth_token'] = ydb_token
    rtlog.activate(file_name, 'notificator')
    Logger.register_rtlog_handler(rtlog.RTLogHandler())
    GlobalCountersUpdater.register(_update_counters)
