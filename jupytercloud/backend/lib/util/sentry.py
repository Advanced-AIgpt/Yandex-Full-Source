import logging

import sentry_sdk
import sentry_sdk.debug
from sentry_sdk.integrations.logging import LoggingIntegration, ignore_logger
from sentry_sdk.integrations.tornado import TornadoIntegration
from traitlets.config.application import Application

from .logging import get_logging_context


def clear_log_data(data):
    if 'color' in data:
        del data['color']
        del data['end_color']

    context = data.get('context')
    if not context:
        data.pop('context', None)


def clear_breadcrumb(breadcrumb, _):
    data = breadcrumb.get('data')
    if data:
        clear_log_data(data)

    return breadcrumb


def filter_events(event, _):
    log_entry = event.get('logentry', {})
    log_message = log_entry.get('message')
    log_params = log_entry.get('params')

    if log_message:
        # do not send event about shutting down via keyboard interrupt
        if (
            log_message.startswith('Received signal %s') and
            log_params and log_params[0] == 'SIGINT'
        ):
            return None

        # it duplicates exception event
        if log_message.startswith('Unhandled error starting '):
            return None

    return event


def obtain_context_from_hint(hint):
    # Если исключение возникло в рамках QYPSpawner или любого другого нашего класса,
    # мы дописываем в него атрибут context.
    # Так, во время рерайза или обработки исключения у нас уже может не быть
    # текущего контекста, тогда мы его достаем из исключения.

    if hint and isinstance(hint, tuple) and len(hint) > 2:
        exception = hint[1]

        if isinstance(exception, BaseException) and hasattr(exception, 'context'):
            return exception.context

    return None


def add_context(event, hint):
    context = get_logging_context() or obtain_context_from_hint(hint) or {}

    extra = event.setdefault('extra', {})
    extra.update(context)
    clear_log_data(extra)

    user_info = event.setdefault('user', {})
    username = context.get('username')
    if user_info.get('username') is None and username:
        user_info['username'] = username
        user_info['email'] = f'{username}@yandex-team.ru'

    return event


def setup_sentry(dsn, environment_name, debug=False):
    app = Application.instance()
    app_name = app.__class__.__name__

    sentry_sdk.init(
        dsn,
        ca_certs='/etc/ssl/certs/ca-certificates.crt',
        integrations=[
            LoggingIntegration(level=logging.DEBUG),
            TornadoIntegration(),
        ],
        environment=environment_name,
        ignore_errors=[KeyboardInterrupt],
        dist=app_name,
        before_send=filter_events,
        before_breadcrumb=clear_breadcrumb,
        send_default_pii=True,
        debug=debug,
    )

    with sentry_sdk.configure_scope() as scope:
        scope.set_tag('application', app_name)
        scope.add_event_processor(filter_events)
        scope.add_event_processor(add_context)
        scope.add_error_processor(add_context)

    ignore_logger('asyncio')
