import logging
from copy import deepcopy
from datetime import datetime

from traitlets import Instance, List, default
from traitlets.config import Configurable, SingletonConfigurable


REPORT_LOGGER_NAME = 'jupytercloud.backend.report'
SUCCESS = 42


class ReportBase(SingletonConfigurable):
    pass


class Report(ReportBase):
    _data = List()  # full list of events

    def _log(self, level, event, message='', **kwargs):
        template = '%s: %s'
        if kwargs:
            template += ' (extra=%s)'
        self.log.log(level, template, event, message, extra=kwargs)

        record = self._process_record(
            {'event': event, 'message': message, 'level': level, **kwargs},
        )
        self._add_record(record)

    def debug(self, *args, **kwargs):
        self._log(logging.DEBUG, *args, **kwargs)

    def info(self, *args, **kwargs):
        self._log(logging.INFO, *args, **kwargs)

    def error(self, *args, **kwargs):
        self._log(logging.ERROR, *args, **kwargs)

    def success(self, *args, **kwargs):
        self._log(SUCCESS, *args, **kwargs)

    def dump(self):
        return deepcopy(self._data)

    def _add_record(self, record):
        self._data.append(record)

    @staticmethod
    def _process_record(record):
        if 'time' not in record:
            t = datetime.now()
            record['time'] = t.time().isoformat(timespec='milliseconds')
        return record


class NullReport(Report):
    def _log(self, *args, **kwargs):
        pass

    def dump(self):
        return []


class ReportMixin(Configurable):
    report = Instance(ReportBase)

    @default('report')
    def _report_default(self):
        if ReportBase.initialized():
            return ReportBase.instance(parent=self)
        else:
            return NullReport.instance(parent=self)
