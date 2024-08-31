import tornado.gen

from . import EventProcessor, register_event_processor
from alice.uniproxy.library.backends_bio import YabioStream
from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.extlog import AccessLogger


class _YabioBase(EventProcessor):
    def __init__(self, directive_name, stream_type, *args, **kwargs):
        super(_YabioBase, self).__init__(*args, **kwargs)
        self.stream = None

        self.stream_type = stream_type

        self.directive_name = directive_name

        self.logger = AccessLogger("yabio", self.system, rt_log=self.rt_log)

        self.data_size = 0

    def close(self):
        super().close()
        if self.stream:
            self.stream.close()
        self.stream = None

    def process_streamcontrol(self, _):
        self.stream.add_chunk(last_chunk=True)
        return EventProcessor.StreamControlAction.Close

    def add_data(self, data):
        self.data_size += len(data)

        if self.stream:
            self.stream.add_chunk(data)

    def on_result(self, result, processed_chunks):
        if self.logger:
            self.logger.end(code=(200 if result.get("status") == "ok" else 500), size=self.data_size)
            self.logger = None
        self.write_directive(self.directive_name, result)
        self.close()

    def write_directive(self, name, result):
        self.system.write_directive(Directive("Biometry", name, result, self.event.message_id))

    def process_event(self, event):
        super(_YabioBase, self).process_event(event)

        self.logger.start(
            event_id=event.message_id,
            resource=self.stream_type.value
        )

        d = self.system.session_data.copy()
        d.update(event.payload)
        self.stream = YabioStream(
            self.stream_type,
            self.on_result,
            self.dispatch_error,
            d,
            host=self.system.srcrwr['YABIO'],
            session_id=self.system.session_id,
            rt_log=self.rt_log,
            system=self.system,
            message_id=event.message_id,
            stream_id=event.stream_id,
        )


@register_event_processor
class Identify(_YabioBase):
    def __init__(self, *args, **kwargs):
        super(Identify, self).__init__("IdentifyComplete", YabioStream.YabioStreamType.Score, *args, **kwargs)


@register_event_processor
class Classify(_YabioBase):
    def __init__(self, *args, **kwargs):
        super(Classify, self).__init__("Classification", YabioStream.YabioStreamType.Classify, *args, **kwargs)


class YabioContextProcessor(EventProcessor):
    def __init__(self, directive_name, resource, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.directive_name = directive_name
        self.resource = resource
        self.group_id = None

    @tornado.gen.coroutine
    def process_event(self, event):
        try:
            super().process_event(event)

            logger = AccessLogger("yabio", self.system, rt_log=self.rt_log)
            logger.start(
                event_id=event.message_id,
                resource=self.resource,
            )

            params = self.system.session_data.copy()
            params.update(event.payload)

            self.group_id = YabioStream.get_group_id(params)
            context_storage = self.system.get_yabio_storage(self.group_id)
            try:
                result = yield self.process_context(context_storage, params)
                log_code = 200
            except Exception as exc:
                self.WARN('yabio fail {}: {}'.format(self.__class__.__name__, str(exc)))
                log_code = 500
                result = {'status': 'failed', 'error': str(exc)}

            logger.end(code=log_code)
            self.system.write_directive(Directive('Biometry', self.directive_name, result, self.event.message_id))
        finally:
            self.close()


@register_event_processor
class CreateOrUpdateUser(YabioContextProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__('UserCreation', 'yabio-create-user', *args, **kwargs)

    @tornado.gen.coroutine
    def process_context(self, context_storage, params):
        yield context_storage.add_user(params['user_id'], params['request_ids'])
        return {'status': 'ok'}


@register_event_processor
class GetUsers(YabioContextProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__('Users', 'yabio-get-users', *args, **kwargs)

    @tornado.gen.coroutine
    def process_context(self, context_storage, params):
        users = yield context_storage.get_users()
        return {'status': 'ok', 'users': users}


@register_event_processor
class RemoveUser(YabioContextProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__('UserRemoved', 'yabio-remove-user', *args, **kwargs)

    @tornado.gen.coroutine
    def process_context(self, context_storage, params):
        yield context_storage.remove_user(params['user_id'])
        return {'status': 'ok'}
