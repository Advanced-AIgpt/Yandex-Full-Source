# coding: utf-8
from __future__ import absolute_import

import os
import time
import logging
import shutil
from threading import Thread
from weakref import ref
from tempfile import NamedTemporaryFile

from fasteners import InterProcessLock, try_lock

from vins_core.utils.data import safe_remove
from vins_core.utils.metrics import sensors


logger = logging.getLogger(__name__)

_updaters = []


def start_all_updaters():
    for upd_ref in _updaters:
        upd = upd_ref()
        if upd is not None:
            upd.start()


class Updater(Thread):
    def __init__(self, file_path, on_update_func=None, interval=10, fall_on_update_error=False):
        super(Updater, self).__init__(name='%s-%s' % (self.__class__.__name__, id(self)))
        self.daemon = True
        self._stopped = False
        self._interval = float(interval)
        self._file_path = file_path
        self._on_update_func = on_update_func
        self._has_update = False
        self._fall_on_update_error = fall_on_update_error
        _updaters.append(ref(self))
        try:
            if self._try_update():
                self.on_update()
        except Exception:
            logger.error('Unhandled updater exception during initialization', exc_info=True)
            if self._fall_on_update_error:
                raise

    @property
    def file_path(self):
        return self._file_path

    def _run_downloader(self):
        while not self._stopped:
            try:
                if self._try_update():
                    self.on_update()
            except Exception:
                logger.error('Unhandled updater exception', exc_info=True)
            time.sleep(self._interval)

    def on_update(self):
        self._has_update = True

    def update(self):
        if self._on_update_func and self._has_update:
            self._on_update_func(self._file_path)
            self._has_update = False

    def _run_watcher(self):
        last_modified = os.path.getctime(self._file_path) if os.path.exists(self._file_path) else 0
        while not self._stopped:
            if os.path.exists(self._file_path):
                modified_time = os.path.getmtime(self._file_path)
                if last_modified < modified_time:
                    try:
                        self.on_update()
                    except Exception:
                        logger.error('Unhandled updater exception', exc_info=True)
                    else:
                        last_modified = modified_time
            time.sleep(self._interval)

    def run(self):
        with try_lock(InterProcessLock(self._file_path + '.lock')) as was_locked:
            if was_locked:
                self._run_downloader()
            else:
                self._run_watcher()

    def stop(self):
        self._stopped = True
        self.join()

    def _download_to_file(self, file_obj):
        """Should return True if new update downloaded

        """
        raise NotImplementedError

    def _try_update(self):
        updated = False
        upd_name = self.__class__.__name__

        f = NamedTemporaryFile(delete=False)
        try:
            with f, sensors.timer('updater_get_update_time', labels={'updater_name': upd_name}):
                updated = self._download_to_file(f)
            if updated:
                sensors.inc_counter('updater_has_update', labels={'updater_name': upd_name})
                shutil.move(f.name, self._file_path)
        finally:
            if os.path.exists(f.name):
                safe_remove(f.name)
        return updated
