# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import attr
import logging

logger = logging.getLogger(__name__)


@attr.s
class AnaphoricContext(object):
    anaphor = attr.ib()
    antecedents = attr.ib()
    same_request_mentions = attr.ib()
    senders = attr.ib()
