import enum


@enum.unique
class VMStatus(enum.Enum):
    wrong_status = -3
    not_exists = -2
    unknown = -1
    empty = 0
    configured = 1
    stopped = 2
    running = 3
    busy = 4
    preparing = 5
    freezed = 6
    crashed = 7
    invalid = 8
    ready_to_start = 9

    # not QYP statuses
    poll_error = -4

    @property
    def is_terminal(self):
        return self in {
            self.running,
            self.stopped,
            self.freezed,
            self.crashed,
            self.invalid,
            self.not_exists,
            # XXX: При внезапном появлении нового статуса, мы не хотим
            # тушить все виртуалки.
            # self.wrong_status
        }

    @property
    def is_startable(self):
        return self in {
            self.running,
            self.stopped,
            self.not_exists,
        }

    @property
    def is_running(self):
        return self == self.running

    @property
    def is_not_exists(self):
        return self == self.not_exists

    @property
    def is_stopped(self):
        return self == self.stopped

    @property
    def is_invalid(self):
        return self == self.invalid

    @property
    def is_wrong(self):
        return self == self.wrong_status

    @classmethod
    def status_by_name(cls, status):
        if isinstance(status, cls):
            return status

        return getattr(
            cls,
            status.lower(),
            cls.wrong_status,
        )

    @classmethod
    def coerce_list(cls, statuses):
        if not isinstance(statuses, (list, tuple)):
            statuses = [statuses]

        return [cls.status_by_name(status) for status in statuses]
