import rtlog
from threading import Thread
from vins_core.utils.metrics import sensors
from time import sleep


_thread = None


def _do_update_counters():
    last_rtlog_stats = rtlog.ClientStats()

    while True:
        stats = rtlog.get_stats()
        sensors.set_sensor('rtlog_active_loggers', stats.active_loggers_count)
        sensors.set_sensor('rtlog_events', stats.events_count - last_rtlog_stats.events_count)
        sensors.set_sensor('rtlog_pending_bytes', stats.pending_bytes_count)
        sensors.set_sensor('rtlog_written_frames', stats.written_frames_count - last_rtlog_stats.written_frames_count)
        sensors.set_sensor('rtlog_written_bytes', stats.written_bytes_count - last_rtlog_stats.written_bytes_count)
        sensors.set_sensor('rtlog_errors', stats.errors_count - last_rtlog_stats.errors_count)
        sensors.set_sensor('rtlog_shrinked_bytes', stats.shrinked_bytes_count - last_rtlog_stats.shrinked_bytes_count)
        last_rtlog_stats = stats

        sleep(1)


def update_counters():
    global _thread

    if not (_thread and _thread.is_alive()):
        _thread = Thread(target=_do_update_counters, name='rtlog_counters_updater')
        _thread.daemon = True
        _thread.start()
