# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import yatest.common as yc


def test_svn_revision(kernel_path):
    result = yc.execute(
        [kernel_path, 'arcadia-info', 'svn-revision']
    )
    std_out = result.std_out.strip()

    if std_out.isdigit():
        # svn revision must be positive
        assert int(std_out) > 0
    else:
        # arc hash should contains only hex digits
        assert std_out.isalnum()
