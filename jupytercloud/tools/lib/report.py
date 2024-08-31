# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import threading
import contextlib
import textwrap

from six import StringIO


_global_report = None


@contextlib.contextmanager
def global_report(marks=()):
    global _global_report
    assert not _global_report
    try:
        _global_report = VMReport(marks=marks)
        yield _global_report
    finally:
        _global_report = None


def get_global_report(assert_exists=True):
    global _global_report
    if assert_exists:
        assert _global_report
    return _global_report


class VMReport(object):
    def __init__(self, marks=()):
        self._vms = {}
        self._errors = {}
        self._marks = set(marks)
        self._lock = threading.RLock()

    def set_mark(self, mark, vm):
        with self._lock:
            self._marks.add(mark)
            vm_info = self._vms.setdefault(vm, {})
            vm_info[mark] = True

    def set_mark_list(self, mark, vms):
        with self._lock:
            self._marks.add(mark)

        for vm in vms:
            self.set_mark(mark, vm)

    def add_error(self, vm, category, error_type, text, **extra):
        self.set_mark('error-{}'.format(error_type), vm)

        info = extra.copy()
        info['category'] = category
        info['error_type'] = error_type,
        info['text'] = text

        with self._lock:
            self._errors.setdefault(vm, [])
            self._errors[vm].append(info)

    def add_error_list(self, vms, category, error_type, text, **extra):
        for vm in vms:
            self.add_error(
                vm=vm, category=category, error_type=error_type,
                text=text, **extra
            )

    def aggregated(self, marks=()):
        if marks:
            marks = set(marks)
            unknown_marks = marks - self._marks
            assert not unknown_marks, unknown_marks
        else:
            marks = self._marks

        report = {}

        for mark in marks:
            report[mark] = 0

            for vm_info in self._vms.values():
                if vm_info.get(mark):
                    report[mark] += 1

        return report

    @property
    def have_errors(self):
        return bool(self._errors)

    def error_report(self):
        result = StringIO()

        def indent(nu, text):
            return '\n'.join(textwrap.wrap(
                text=text,
                width=120,
                initial_indent=' ' * nu,
                subsequent_indent=' ' * nu,
            ))

        for vm in sorted(self._errors):
            errors = self._errors[vm]

            print('* VM: {}'.format(vm), file=result)
            for error in errors:
                print('  - Category: {}'.format(error['category']), file=result)
                print('    Error-type: {}'.format(error['error_type']), file=result)
                print('    Text:', file=result)
                print(indent(8, error['text']), file=result)

        return result.getvalue()
