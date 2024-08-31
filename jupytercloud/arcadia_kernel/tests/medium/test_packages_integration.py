# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals


def test_nile_ipython_feature(execute):
    execute("import nile.style.jupyter_monitor")
    execute("assert nile.style.jupyter_monitor.widgets")


def test_nile_in_jupyter(execute):
    # XXX Вообще, это не дожно работать без самого jupyter,
    # но из-за несовершенства проверки это работает, т.к.
    # мы выполняемся в ipykernel.
    execute("from nile.utils.misc import in_jupyter")
    execute("assert in_jupyter()")


def test_pyplot_integration(execute):
    execute("import matplotlib.pyplot as plt")
    execute("plt.plot([1.6, 2.7])")
