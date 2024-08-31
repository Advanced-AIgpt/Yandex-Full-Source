import re
import collections.abc
import functools
from alice.uniproxy.library.utils.deepcopyx import deepcopy
from distutils.version import LooseVersion

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils.experiments import safe_experiments_vins_format
from alice.uniproxy.library.utils.experiments import weak_update_experiments


def parse_path(path):
    if isinstance(path, (list, tuple)):
        return path

    if not isinstance(path, str):
        raise Exception('bad path format (expected list of strings of string): {}'.format(str(path)))

    if not path:
        raise Exception('empty path string (MUST contain at least one separator char)')

    if path[0] not in './@':
        raise Exception('bad first path char (expected separator - one of "./@" chars): "{}"'.format(path[0]))

    if len(path) == 1:
        return []

    return path[1:].split(path[0])


def set_by_path(root, path, value, overwrite=True):
    if len(path) == 1:
        if overwrite or path[0] not in root:
            root[path[0]] = deepcopy(value)
        return

    if path[0] not in root or not isinstance(root[path[0]], dict):
        root[path[0]] = {}
    set_by_path(root[path[0]], path[1:], value, overwrite)


def tricky_append_by_path(root, path, value):
    if path == ['request', 'experiments']:
        set_by_path(root, path + [value], '1', overwrite=False)
    else:
        append_by_path(root, path, value)


def append_by_path(root, path, value):
    if len(path) == 1:
        if path[0] not in root:
            root[path[0]] = [deepcopy(value), ]
        elif isinstance(root[path[0]], list) and value not in root[path[0]]:
            root[path[0]].append(deepcopy(value))
        return

    if path[0] not in root or not isinstance(root[path[0]], dict):
        root[path[0]] = {}
    append_by_path(root[path[0]], path[1:], value)


def tricky_extend_by_path(root, path, value):
    if path == ['request', 'experiments']:
        new_experiments = safe_experiments_vins_format(value, Logger.get().warning)
        cur_experiments = root.setdefault("request", {}).setdefault("experiments", {})
        weak_update_experiments(cur_experiments, new_experiments)
    else:
        extend_by_path(root, path, value)


def extend_by_path(root, path, value):
    if len(path) == 1:
        if path[0] not in root:
            root[path[0]] = list(deepcopy(value))
        elif isinstance(root[path[0]], list):
            root[path[0]].extend(filter(lambda x: x not in root[path[0]], value))
        return

    if path[0] not in root or not isinstance(root[path[0]], dict):
        root[path[0]] = {}
    extend_by_path(root[path[0]], path[1:], value)


def del_by_path(root, path):
    if not root or not isinstance(root, dict):
        return

    if len(path) == 1:
        root.pop(path[0], None)
        return

    del_by_path(root.get(path[0]), path[1:])


def has_path(root, path):
    if not root or not isinstance(root, dict):
        return False

    if len(path) == 1:
        return path[0] in root

    return has_path(root.get(path[0]), path[1:])


def get_by_path(root, path):
    if not root or not isinstance(root, dict):
        return (False, None)

    if len(path) == 1:
        if path[0] not in root:
            return (False, None)
        return (True, root[path[0]])

    return get_by_path(root.get(path[0]), path[1:])


def check_args_num(num):
    def wrap(func):
        assert func.__name__.startswith('cmd_')

        @functools.wraps(func)
        def wrapped(self, event, args):
            if len(args) < num:
                raise Exception('command <{}> has less than {} param(s)'.format(func.__name__[4:], num))
            return func(self, event, args)
        return wrapped
    return wrap


class EventPatcher:
    """
        update events.Event instance using script (parsed response) from UaaS
    """
    def __init__(self, flags, macros):
        self.flags = flags
        self.session_data = None
        self.staff_login = None
        self.macros = deepcopy(macros)

    def useful(self):
        return self.flags

    def patch(self, event, session_data, hide_exceptions=True, staff_login=None, rt_log=None, backup=True):
        """
            apply to event (for own handling errors can use hide_exception=False)
        """
        flags = []
        if backup:
            orig_payload = deepcopy(event.payload)
        try:
            if self.flags is None or not isinstance(self.flags, (list, tuple)):
                raise Exception('unexpected flags: {}'.format(str(self.flags)))

            if not isinstance(event.payload, collections.abc.Mapping):
                raise Exception('unexpected event.payload (expect dict): {}'.format(str(event.payload)))

            for flag in self.flags:
                if not isinstance(flag, (list, tuple)):
                    raise Exception('unexpected flag[{}] inside flags: {}'.format(str(flag), str(self.flags)))
                self.session_data = session_data
                self.staff_login = staff_login
                if self.apply_flag(event, flag):
                    flags.append(flag)
        except Exception as exc:
            flags = []
            if backup:
                event.payload = orig_payload
            if not hide_exceptions:
                raise
            log = Logger.get('.event.patcher')
            log.error('has exception on apply flags={}: {}'.format(str(self.flags), repr(exc)), rt_log=rt_log)
            log.exception(exc, rt_log=rt_log)
        finally:
            self.session_data = None
            if hide_exceptions:
                return flags

    def apply_flag(self, event, flag):
        if not flag:
            return None

        cmd = flag[0]
        method = self.__class__.__dict__.get('cmd_{}'.format(cmd))
        if method is None:
            raise Exception('unknown command: {}'.format(cmd))
        else:
            return method(self, event, flag[1:])

    @check_args_num(2)
    def cmd_set(self, event, args):
        path = parse_path(args[0])
        if not path:
            event.payload = args[1]
        else:
            set_by_path(event.payload, path, args[1])
        r = self.apply_flag(event, args[2:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_set_if_none(self, event, args):
        path = parse_path(args[0])
        if not path:
            event.payload = args[1]
        else:
            set_by_path(event.payload, path, args[1], overwrite=False)
        r = self.apply_flag(event, args[2:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_append(self, event, args):
        path = parse_path(args[0])
        tricky_append_by_path(event.payload, path, args[1])
        r = self.apply_flag(event, args[2:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_extend(self, event, args):
        if not isinstance(args[1], (list, tuple)):
            raise Exception('command <extend> MUST has 2nd parameter as iterable')

        path = parse_path(args[0])
        tricky_extend_by_path(event.payload, path, args[1])
        r = self.apply_flag(event, args[2:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_import_macro(self, event, args):
        if args[1] not in self.macros:
            raise Exception('command <import_macro got missing macro {}'.format(args[1]))

        path = parse_path(args[0])
        tricky_extend_by_path(event.payload, path, self.macros.get(args[1]))
        r = self.apply_flag(event, args[2:])
        return r if r is not None else True

    @check_args_num(1)
    def cmd_del(self, event, args):
        path = parse_path(args[0])
        if not path:
            raise Exception('command <del> MUST has not empty path as argument')

        del_by_path(event.payload, path)
        r = self.apply_flag(event, args[1:])
        return r if r is not None else True

    @check_args_num(1)
    def cmd_if_event_type(self, event, args):
        if event.event_type() != args[0].lower():
            return False  # not fulfilled the condition, stop processing flag

        r = self.apply_flag(event, args[1:])
        return r if r is not None else True

    def cmd_if_has_staff_login(self, event, args):
        if not self.staff_login:
            return False

        r = self.apply_flag(event, args)
        return r if r is not None else True

    @check_args_num(1)
    def cmd_if_staff_login_eq(self, event, args):
        if self.staff_login != args[0]:
            return False

        r = self.apply_flag(event, args[1:])
        return r if r is not None else True

    @check_args_num(1)
    def cmd_if_staff_login_in(self, event, args):
        if not isinstance(args[0], (list, tuple)):
            raise Exception('command <if_staff_login_in> MUST has 1st parameter as iterable')
        if self.staff_login not in args[0]:
            return False

        r = self.apply_flag(event, args[1:])
        return r if r is not None else True

    @check_args_num(1)
    def cmd_if_has_payload(self, event, args):
        path = parse_path(args[0])
        if path and not has_path(event.payload, path) and not has_path(self.session_data, path):
            return False

        r = self.apply_flag(event, args[1:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_if_payload_eq(self, event, args):
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(event.payload, path)
            if not has_value:
                has_value, value = get_by_path(self.session_data, path)
                if not has_value or value != args[1]:
                    return False
            elif value != args[1]:
                return False
            r = self.apply_flag(event, args[2:])
            return r if r is not None else True
        return False

    @check_args_num(2)
    def cmd_if_payload_ne(self, event, args):
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(event.payload, path)
            if has_value:
                if value == args[1]:
                    return False
            else:
                has_value, value = get_by_path(self.session_data, path)
                if has_value and value == args[1]:
                    return False
        r = self.apply_flag(event, args[2:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_if_payload_in(self, event, args):
        if not isinstance(args[1], (list, tuple)):
            raise Exception('command <if_payload_in> MUST has 2nd parameter as iterable')
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(event.payload, path)
            if not has_value:
                has_value, value = get_by_path(self.session_data, path)
                if not has_value or value not in args[1]:
                    return False
            elif value not in args[1]:
                return False
            r = self.apply_flag(event, args[2:])
            return r if r is not None else True
        return False

    @check_args_num(2)
    def cmd_if_payload_like(self, event, args):
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(event.payload, path)
            if not has_value:
                has_value, value = get_by_path(self.session_data, path)
                if not has_value or not re.match(args[1], value):
                    return False
            elif not re.match(args[1], value):
                return False
            r = self.apply_flag(event, args[2:])
            return r if r is not None else True
        return False

    @check_args_num(2)
    def cmd_if_payload_loose_version_gt(self, event, args):
        return self.if_payload_loose_version(event, args, 'gt')

    def if_payload_loose_version(self, event, args, cmp_op):
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(event.payload, path)
            if not has_value:
                has_value, value = get_by_path(self.session_data, path)
                if not has_value or value != args[1]:
                    return False
            elif cmp_op == 'gt':
                if LooseVersion(value) <= LooseVersion(args[1]):
                    return False
            elif cmp_op == 'ge':
                if LooseVersion(value) < LooseVersion(args[1]):
                    return False
            elif cmp_op == 'lt':
                if LooseVersion(value) >= LooseVersion(args[1]):
                    return False
            elif cmp_op == 'le':
                if LooseVersion(value) > LooseVersion(args[1]):
                    return False

            r = self.apply_flag(event, args[2:])
            return r if r is not None else True
        return False

    @check_args_num(2)
    def cmd_if_payload_loose_version_ge(self, event, args):
        return self.if_payload_loose_version(event, args, 'ge')

    @check_args_num(2)
    def cmd_if_payload_loose_version_lt(self, event, args):
        return self.if_payload_loose_version(event, args, 'lt')

    @check_args_num(2)
    def cmd_if_payload_loose_version_le(self, event, args):
        return self.if_payload_loose_version(event, args, 'le')

    @check_args_num(2)
    def cmd_if_session_data_like(self, event, args):
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(self.session_data, path)
            if not has_value or not re.match(args[1], value):
                return False
            r = self.apply_flag(event, args[2:])
            return r if r is not None else True
        return False

    @check_args_num(1)
    def cmd_if_has_session_data(self, event, args):
        path = parse_path(args[0])
        if path and not has_path(self.session_data, path):
            return False

        r = self.apply_flag(event, args[1:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_if_session_data_eq(self, event, args):
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(self.session_data, path)
            if not has_value or value != args[1]:
                return False
            r = self.apply_flag(event, args[2:])
            return r if r is not None else True
        return False

    @check_args_num(2)
    def cmd_if_session_data_ne(self, event, args):
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(self.session_data, path)
            if has_value and value == args[1]:
                return False
        r = self.apply_flag(event, args[2:])
        return r if r is not None else True

    @check_args_num(2)
    def cmd_if_session_data_in(self, event, args):
        if not isinstance(args[1], (list, tuple)):
            raise Exception('command <if_session_data_in> MUST has 2nd parameter as iterable')
        path = parse_path(args[0])
        if path:
            has_value, value = get_by_path(self.session_data, path)
            if not has_value or value not in args[1]:
                return False
            r = self.apply_flag(event, args[2:])
            return r if r is not None else True
        return False


class EventPatcherCondition(EventPatcher):
    def __init__(self, conditions):
        super().__init__([conditions], {})

    # implement only evaluating if* conditions, whithout event modification
    def eval_condition(self, event, session_data, staff_login):
        if self.flags is None or not isinstance(self.flags, (list, tuple)) or len(self.flags) == 0:
            raise Exception('unexpected flags: {}'.format(str(self.flags)))

        if not isinstance(event.payload, collections.abc.Mapping):
            raise Exception('unexpected event.payload (expect dict): {}'.format(str(event.payload)))

        for flag in self.flags:
            if not isinstance(flag, (list, tuple)):
                raise Exception('unexpected flag inside flags: {}'.format(str(self.flags)))

            self.session_data = session_data
            self.staff_login = staff_login
            if not self.apply_flag(event, flag):
                return False

        return True

    def apply_flag(self, event, flag):
        if not flag:  # empty flags list mean we confirmed all gived conditions
            return True

        cmd = flag[0]
        method = EventPatcher.__dict__.get('cmd_{}'.format(cmd))
        if method is None:
            raise Exception('unknown command: {}'.format(cmd))
        else:
            if not cmd.startswith('if_'):
                raise Exception('EventPatcherCondition allow use only if_* conditions (methods for modify event forbidded), buf got: {}'.format(cmd))
            return method(self, event, flag[1:])


class MultiPatcher:
    def __init__(self):
        self.event_patchers = []
        self.empty = True

    def add_patcher(self, patcher, exp=None):
        self.empty = False  # can be not empty even of not have patchers (for control exp-s)
        self.event_patchers.append((exp, patcher))

    def _patch(self, *args, **kwargs):
        ret = []
        for exp, patcher in self.event_patchers:
            flags = patcher.patch(*args, **kwargs)
            if flags:
                ret.append((exp, flags))
        return ret

    def patch(self, event, session_data, hide_exceptions=True, staff_login=None, rt_log=None):
        orig_payload = deepcopy(event.payload)  # single backup copy wor all patchers
        try:
            return self._patch(event, session_data, hide_exceptions=False, staff_login=staff_login, rt_log=rt_log, backup=False)
        except Exception as err:
            log = Logger.get('.event.patcher')
            log.error("Exception in one of patchers: {}; repeat with per-patcher backup".format(err))
            event.payload = orig_payload
        return self._patch(event, session_data, hide_exceptions=hide_exceptions, staff_login=staff_login, rt_log=rt_log)

    def useful(self):
        for _, patcher in self.event_patchers:
            if patcher.useful():
                return True
        return False
