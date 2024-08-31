# coding: utf-8
from __future__ import unicode_literals

from jinja2 import contextfunction

import logging

logger = logging.getLogger(__name__)


def _get_session(context):
    return context['session']


@contextfunction
def is_first_message(context):
    session = _get_session(context)
    first_request = session.get('first_request', 0)
    if first_request:
        session.set('first_request', 0)
    return first_request


def redirect_prefix(context):
    prefix = ''
    if context['req_info'].experiments['increased_response_time'] is not None:
        prefix += "\\n\\n" \
                  "Сейчас у нас больше обращений, чем обычно, поэтому мы можем отвечать дольше. " \
                  "Постараемся ответить вам в ближайшее время. А ещё вы можете написать нам обращение через "
        if is_webim(context):
            prefix += "[форму обратной связи](https://beru.ru/help/feedback.html)"
        else:
            prefix += "форму обратной связи на сайте: https://beru.ru/help/feedback.html"
        prefix += "\\n\\n" \
                  "Извините за неудобства, не болейте и берегите себя! "
    return prefix


@contextfunction
def default_redirect(context):
    session = _get_session(context)
    session.set('doing_redirect', True)
    return redirect_prefix(context) + 'OPERATOR_REDIRECT'


@contextfunction
def operator_redirect(context, operator_id):
    session = _get_session(context)
    session.set('doing_redirect', True)
    return redirect_prefix(context) + 'OPERATOR_REDIRECT_{}'.format(operator_id)


@contextfunction
def department_redirect(context, dep_key):
    session = _get_session(context)
    session.set('doing_redirect', True)
    return redirect_prefix(context) + 'OPERATOR_REDIRECT_{}'.format(dep_key)


@contextfunction
def is_webim_v1(context):
    app_info = context['req_info'].app_info
    return app_info.app_id == "webim_crm_bot" and app_info.app_version == "1.0"


@contextfunction
def is_webim_v2(context):
    app_info = context['req_info'].app_info
    return app_info.app_id == "webim_crm_bot" and app_info.app_version == "2.0"


@contextfunction
def is_webim(context):
    return context['req_info'].app_info.app_id == "webim_crm_bot"
