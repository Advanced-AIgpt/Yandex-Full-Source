import asyncio
import aiohttp
import collections
import datetime
import uuid
import time
import struct
import json
import os

from .actions import *          # noqa
from .checks_proxy import *     # noqa

from .context import Context
from .actions import Action, ActionBase, action
from .actions_scenario import Scenario


from library.python import resource


class ProxyDirectiveDispatcher(object):
    def __init__(self, context: Context):
        self._context = context
        self._stop = False
        self._stopped_future = asyncio.Future()
        self._ids = {}
        self._msgids = {}
        self._messages = collections.defaultdict(list)
        self._task = asyncio.create_task(self.task_main(self._context))

    def add_operation(self, operation_id, message_id):
        self._ids[operation_id] = message_id

    async def wait_for_message(self, handler, message_id=None, operation_id=None):
        if message_id is None:
            if operation_id is not None:
                if operation_id in self._ids:
                    message_id = self._ids[operation_id]

            if message_id is None:
                message_id = self._context['session'].last_message_id

        if message_id is None:
            self._context.warning('failed to find out which message id to wait')
            return False

        if message_id in self._messages:
            for message in self._messages[message_id]:
                await handler.on_message(message)

        self._msgids[message_id] = handler

        return True

    async def remove_wait(self, message_id=None, operation_id=None, handler=None):
        self._context.log.debug(
            'remove_wait message=%s operation=%s handler=%s',
            message_id,
            operation_id,
            handler
        )

        if operation_id and operation_id in self._ids:
            message_id = self._ids[operation_id]
            del self._ids[operation_id]

        if message_id and message_id in self._msgids:
            del self._msgids[message_id]

        if message_id and message_id in self._messages:
            del self._messages[message_id]

        return True

    async def dispatch_closing(self, context):
        for k, handler in self._msgids.items():
            await handler.on_close()

        self._ids = {}
        self._msgids = {}
        self._messages = collections.defaultdict(list)

    async def dispatch_message(self, message, context: Context):
        if 'streamcontrol' in message:
            await self.dispatch_streamcontrol(message, context)
        elif 'directive' in message:
            await self.dispatch_directive(message, context)
        else:
            context.log.error('unexpected message: %s', message)

    async def dispatch_stream(self, message, context: Context):
        context.log.error('dispatch_stream is not implemented')
        if len(message) >= 4:
            stream_no, data = struct.unpack('>I', message[:4]), len(message[4:])
            context.log.info('STREAM: No %d LEN %d', stream_no[0], data)

    async def dispatch_directive(self, message, context: Context):
        message_id = message.get('directive', {}).get('header', {}).get('refMessageId')
        if message_id and message_id in self._msgids:
            await self._msgids[message_id].on_message(message)
        elif message_id:
            self._messages[message_id].append(message)

        if context.debug_directives:
            context.log.info('DIRECTIVE: %s', message.get('directive'))

    async def dispatch_streamcontrol(self, message, context: Context):
        context.log.error('dispatch_streamcontrol is not implemented')
        if context.debug_directives:
            context.log.info('STREAMCONTROL: %s', message)

    async def task_main(self, context: Context):
        try:
            context.log.debug('ProxyDirectiveDispatcher started')
            while not self._stop:
                try:
                    msg = await asyncio.wait_for(context.connection.receive(), 0.25)
                    if msg is None:
                        context.log.info('connection closed')
                        self._stop = True
                    elif msg.type == aiohttp.WSMsgType.TEXT:
                        message = json.loads(msg.data)
                        await self.dispatch_message(message, context)
                    elif msg.type == aiohttp.WSMsgType.BINARY:
                        await self.dispatch_stream(msg.data, context)
                    elif msg.type == aiohttp.WSMsgType.CLOSED:
                        context.log.info('connection was closed')
                        self._stop = True
                    elif msg.type == aiohttp.WSMsgType.CLOSING:
                        await self.dispatch_closing(context)
                    else:
                        context.log.warning('unknown message type: %s', msg.type)

                except asyncio.TimeoutError:
                    pass
                except Exception as ex:
                    context.log.error('error while receiving message: %s', ex)
                    break
        except Exception as ex:
            context.log.error('failed to run ProxyDirectiveDispatcher.task_main: %s', ex)

        self._stopped_future.set_result(True)

    async def stop(self):
        self._stop = True
        await self._stopped_future


@action('proxy.session')
class ProxySession(Scenario):
    async def pre_execute(self, context: Context):
        context.set_tag('session')

        if context.uuid is None:
            context.uuid = str(uuid.uuid4())

        if context.token is None:
            context.token = 'developers-simple-key'

        context.session = aiohttp.ClientSession()

        headers = None
        if context.send_headers:
            headers = {
                'X-UPRX-AUTH-TOKEN': context.token,
                'X-UPRX-UUID': context.uuid,
            }

        timeout = context.get('connect_timeout', context.get('ws_connect_timeout'))
        try:
            context.connection = await context.session.ws_connect(
                context.url, timeout=timeout, headers=headers
            )
        except Exception as ex:
            context.log.error('ws_connect -> %s', ex)
            del context.connection

        if context.connection:
            context.log.debug('connected to %s', context.url)
            context.dispatcher = ProxyDirectiveDispatcher(context)
        else:
            context.report.ok = False
            context.report.message = f'failed to connect to {context.url}'
            context.log.error(context.report.message)
            await context.session.close()

    async def post_execute(self, context: Context):
        if context.stream_tasks:
            context.log.debug('waiting for a streams...')
            for stream_id, task in context.stream_tasks.items():
                try:
                    await task
                except Exception as ex:
                    context.log.error('waiting for a stream failed: %s', ex)

        connection = context.get('connection')
        if connection:
            await connection.close()
            del context.connection

        session = context.get('session')
        if session:
            await session.close()
            del context.session

        dispatcher = context.get('dispatcher')
        if dispatcher:
            await dispatcher.stop()
            del context.distpatcher


@action('proxy.send_event')
class ProxySendEvent(ActionBase):
    async def execute(self, context: Context):
        message_id = str(uuid.uuid4())

        if context.id:
            context.dispatcher.add_operation(context.id, message_id)

        event = {
            'event': {
                'header': {
                    'namespace': context.namespace,
                    'name': context.name,
                    'messageId': message_id,
                },
                'payload': context.payload,
            }
        }

        if context.stream_id:
            event['event']['header']['streamId'] = context.stream_id

        context['session'].last_message_id = message_id

        try:
            context.log.debug(event)
            await context.connection.send_json(event)
        except Exception as ex:
            context.report.ok = False
            context.report.message = f'failed to send event `{context.namespace}.{context.name}`: {ex}'
            return False

        return True


@action('proxy.synchronize_state')
class ProxySynchronizeState(ProxySendEvent):
    async def execute(self, context: Context):
        context.log.debug('executing ProxySynchronizeState...')

        context.namespace = 'System'
        context.name = 'SynchronizeState'
        context.payload = {
            'lang': context.get('lang', 'ru-RU'),
            'uuid': context.uuid,
            'auth_token': context.token,
            'vins': {
                'application': {
                    'uuid': context.uuid.replace('-', ''),
                    'lang': context.get('lang', 'ru-RU'),
                }
            }
        }

        if context.custom_payload:
            context.payload.update(context.custom_payload)

        if context.voice:
            context.payload['voice'] = context.voice

        if context.emotion:
            context.payload['emotion'] = context.emotion

        if context.oauth_token:
            context.payload['oauth_token'] = context.oauth_token

        if context.yandexuid:
            context.payload['yandexuid'] = context.yandexuid

        if context.messenger_version:
            context.payload['Messenger'] = {
                'version': context.messenger_version,
            }

        if context.app_id and context.app_version:
            context.payload['vins']['application']['app_id'] = context.app_id
            context.payload['vins']['application']['app_version'] = context.app_version

        if context.platform:
            context.payload['vins']['application']['platform'] = context.platform

        if context.os_version:
            context.payload['vins']['application']['os_version'] = context.os_version

        if context.device_model:
            context.payload['vins']['application']['device_model'] = context.device_model

        if context.device_manufacturer:
            context.payload['vins']['application']['device_manufacturer'] = context.device_manufacturer

        if context.vins_device_id:
            context.payload['vins']['application']['device_id'] = context.vins_device_id

        if context.experiments:
            context.payload['request'] = context.payload.get('request', {})
            context.payload['request']['experiments'] = context.experiments

        if not context.enable_local_experiments:
            context.payload['disable_local_experiments'] = True

        context.log.debug('delegating action processing to ProxySendEvent...')
        return await super().execute(context)


@action('proxy.send_stream')
class ProxySendStream(ActionBase):
    async def execute_part_common(self, context, data):
        chunk_size = context.get('chunk_size', 8000)
        chunk_duration = context.get('chunk_duration', 0.1)

        try:
            chunk = None
            next_chunk = data
            completed = False
            while not completed:
                chunk = next_chunk[:chunk_size]
                next_chunk = next_chunk[chunk_size:]

                context.log.debug(f'sending chunk stream_id={context.stream_id} size={len(chunk)}')
                stream_id_data = struct.pack('>I', context.stream_id)
                stream_data = stream_id_data + chunk

                await context.connection.send_bytes(stream_data)

                if next_chunk:
                    await asyncio.sleep(chunk_duration)
                else:
                    completed = True
        except Exception as ex:
            context.report.ok = False
            context.report.message = f'failed to send data chunk for stream_id={context.stream_id}: {ex}'
            context.log.error(context.report.message)
            return False

        if not context.leave_open:
            try:
                event = {
                    'streamcontrol': {
                        'streamId': context.stream_id,
                        'messageId': context.message_id,
                        'action': 2 if context.flush else 0,
                        'reason': 0
                    }
                }
                context.log.debug(event)
                await context.connection.send_json(event)
            except Exception:
                context.report.ok = False
                context.report.message = 'failed to send streamcontrol'
                context.log.error(context.report.message)
                return False

        return True

    async def execute_part_from_path(self, context: Context, path):
        path = os.path.relpath(path)
        context.log.debug(f'sending stream from `{path}`...')
        with open(path, 'rb') as fin:
            data = fin.read()
        ret = await self.execute_part_common(context, data)
        context.log.info(f'sent stream from `{path}`')
        return ret

    async def execute_part_from_resource(self, context: Context, path):
        data = resource.find(path)
        context.log.debug(f'sending stream from `res://{path}`...')
        ret = await self.execute_part_common(context, data)
        context.log.debug(f'sent stream from `res://{path}`...')
        return ret

    async def execute_part(self, context: Context):
        audio_path = context.get('filename')
        audio_resource = context.get('resource')

        if not audio_path and not resource:
            context.report.ok = False
            context.report.message = 'neither path nor resource were provided for stream'
            context.log.error(context.report.message)
            return False

        if audio_path:
            return await self.execute_part_from_path(context, audio_path)

        if audio_resource:
            return await self.execute_part_from_resource(context, audio_resource)

        return False

    async def execute(self, context: Context):
        stream_id = context.get('stream_id', context['session'].last_stream_id)
        message_id = context.get('message_id', context['session'].last_message_id)

        if stream_id is None:
            context.report.ok = False
            context.report.message = 'trying to send stream without streamId'
            context.log.error(context.report.message)
            return False

        if message_id is None:
            context.report.ok = False
            context.report.message = 'trying to send stream without assigned messageId'
            context.log.error(context.report.message)
            return False

        if context.stream is None:
            context.report.ok = False
            context.report.message = 'no stream data to send'
            context.log.error(context.report.message)
            return False

        for stream_part in context.stream:
            stream_part['stream_id'] = stream_id
            stream_part['message_id'] = message_id
            await self.execute_part(context('part', data=stream_part))
        return True


@action('proxy.wait_for_stream')
class ProxyWaitForStream(ActionBase):
    async def execute(self, context: Context):
        stream_id = context.get('stream_id', context['session'].last_stream_id)

        session = context['session']

        if stream_id is None:
            context.log.debug('using session.last_stream_id for stream_id')
            stream_id = session.last_stream_id

        if stream_id is None:
            context.log.debug('stream_id is None')
            return True

        task = session.stream_tasks.get(stream_id)
        if task is None:
            context.log.debug('task is None, assuming it has already been waited for')
            return True

        context.log.debug(f'waiting for stream_id={stream_id}')
        try:
            await task
            context.log.debug(f'streaming stream_id={stream_id} was completed')
        except Exception as ex:
            context.report.ok = False
            context.report.message = f'failed to stream stream_id={stream_id}: {ex}'
            context.log.error(context.report.message)
            return False

        return True


@action('proxy.send_event_with_stream')
class ProxySendEventWithStream(ProxySendEvent):
    async def execute(self, context: Context):
        if context.stream is None:
            return await super().execute(context)

        session = context['session']

        if session.next_stream_id is None:
            session.next_stream_id = 1

        context.stream_id = session.next_stream_id
        session.last_stream_id = context.stream_id
        session.next_stream_id += 2

        if session.stream_tasks is None:
            session.stream_tasks = {}

        ret = await super().execute(context)

        action = ProxySendStream(context('stream', tag='stream'))()
        if context.sync_stream:
            await action
        else:
            session.stream_tasks[context.stream_id] = asyncio.create_task(action)

        return ret


@action('proxy.asr_recognize')
class ProxyASRRecognize(ProxySendEventWithStream):
    async def execute(self, context: Context):
        context.namespace = 'ASR'
        context.name = 'Recognize'
        context.payload = {
            'lang': context.get('lang', 'ru-RU'),
            'topic': context.get('topic', 'general'),
            'application': 'proxycli',
            'format': context.get('format', 'audio/opus'),
            'key': context.token,
            'punctuation': context.get('punctuation', False),
        }

        if context.partials is not None:
            opts = context.payload.get('advancedASROptions', {})
            opts['partial_results'] = True if context.partials else False
            context.payload['advancedASROptions'] = opts

        if context.advanced:
            opts = context.payload.get('advancedASROptions', {})
            opts.update({
                k: v for k, v in context.advanced.items()
            })
            context.payload['advancedASROptions'] = opts

        return await super().execute(context)


@action('proxy.vins.voice_input')
class ProxyVinsVoiceInput(ProxySendEventWithStream):
    async def execute(self, context: Context):
        context.namespace = 'Vins'
        context.name = 'VoiceInput'

        vins_request_id = str(uuid.uuid4())
        context.last_vins_request_id = vins_request_id

        now = datetime.datetime.now()

        context.payload = {
            'lang': context.get('lang', 'ru-RU'),
            'topic': context.get('topic', 'general'),
            'format': context.get('format', 'audio/opus'),
            'key': context.token,
            'punctuation': context.get('punctuation', False),
            'uuid': context.get('uuid', '').replace('-', ''),
            'application': {
                'timezone': 'Europe/Moscow',
                'client_time': now.strftime("%Y%m%dT%H%M%S"),
                'timestamp': str(int(now.timestamp())),
            },
            'header': {
                'request_id': vins_request_id,
            },
            'request': {
                'event': {
                    'type': 'text_input' if context.text_input else 'voice_input',
                    'voice_session': False if context.text_input else True
                }
            }
        }

        if context.partials is not None:
            opts = context.payload.get('advancedASROptions', {})
            opts['partial_results'] = True if context.partials else False
            context.payload['advancedASROptions'] = opts

        if context.advanced:
            opts = context.payload.get('advancedASROptions', {})
            opts.update({
                k: v for k, v in context.advanced.items()
            })
            context.payload['advancedASROptions'] = opts

        if context.asr_balancer:
            context.payload['asr_balancer'] = context.asr_balancer

        if context.spotter_validation:
            context.payload['enable_spotter_validation'] = True

            if 'phrase' in context.spotter_validation:
                context.payload['spotter_phrase'] = context.spotter_validation.get('phrase')

            if context.spotter_validation.get('disable', False):
                context.payload['disable_spotter_validation'] = True

            if 'spotter_back' in context.spotter_validation:
                context.payload['spotter_options'] = context.payload.get('spotter_options', {})
                context.payload['spotter_options']['spotter_back'] = context.spotter_validation.get('spotter_back')

            if 'request_front' in context.spotter_validation:
                context.payload['spotter_options'] = context.payload.get('spotter_options', {})
                context.payload['spotter_options']['request_front'] = context.spotter_validation.get('request_front')

        return await super().execute(context)


@action('proxy.debug_recv')
class ProxyDebugRecv(ActionBase):
    async def execute(self, context: Context):
        ts = time.time()
        te = ts + context.get('timeout', 10.0)

        try:
            while True:
                msg = await asyncio.wait_for(context.connection.receive(), te - time.time())
                if msg is None:
                    context.log.info('got EOF message')
                elif msg.type == aiohttp.WSMsgType.BINARY:
                    context.log.info('got BINARY message: %s', msg.data)
                elif msg.type == aiohttp.WSMsgType.TEXT:
                    context.log.info('got TEXT message: %s', json.loads(msg.data))

                if time.time() > te:
                    context.log.info('stopped listening to the messages')
                    break

        except asyncio.TimeoutError:
            context.log.info('stopped listening to the messages')

        except Exception as ex:
            context.log.error('failed to receive some messages: %s', ex)


@action('proxy.wait_for_reply')
class ProxyWaitForReply(ActionBase):
    async def execute(self, context):
        if context.id:
            context.message_id = context.dispacher.get_message_id(context.id)
        elif context.message_id is None:
            context.message_id = context['session'].last_message_id

        if context.message_id is None:
            context.report.ok = False
            context.report.message = 'no message id to wait'
            context.log.error(context.report.message)
            return False

        context.reply = asyncio.Future()

        context.log.debug('waiting for a message %s', context.message_id)
        await context.dispatcher.wait_for_message(self, message_id=context.message_id)

        context.log.debug('no message, still waiting for a message %s', context.message_id)
        return await self.wait_for_reply(context)

    async def on_message(self, message):
        self._context.log.debug('on_message: %s', message)

        results = []
        for check in self._context.checks:
            ctx = self._context('check', tag='check', data=check)
            ctx.message = message
            check_action = Action(ctx.check, ctx)

            if check_action:
                ret = await check_action()
                results.append(ret)
            else:
                self._context.log.error('no check action `%s`', ctx.check)

        self._context.reply.set_result(all(results))

    async def on_close(self):
        self._context.report.ok = False
        self._context.report.message = 'waiting for message=%s failed: connection closing' % (
            self._context.message_id
        )
        self._context.log.error(self._context.report.message)
        self._context.reply.set_result(False)

    async def wait_for_reply(self, context: Context):
        ret = False
        try:
            ret = await asyncio.wait_for(context.reply, context.get('timeout', 10.0))
        except asyncio.TimeoutError:
            context.log.info('waiting for message_id=%s timed out', context.message_id)
            return False
        except Exception as ex:
            context.report.ok = False
            context.report.message = f'waiting for message_id={context.message_id} failed: {ex}'
            context.log.error(context.report.message)
            return False
        finally:
            await context.dispatcher.remove_wait(message_id=context.message_id)
        return ret


class ProxyReplyHandler(ActionBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._done = False
        self._ready = asyncio.Future()

    async def execute(self, context: Context):
        if self._done:
            return await self._ready

        results = []
        for check in context.checks:
            ctx = context('check', tag='check', data=check)
            ctx.message = ctx.wsmsg
            check_action = Action(ctx.check, ctx)

            if check_action:
                ret = await check_action()
                results.append(ret)
            else:
                self._context.log.error('no check action `%s`', ctx.check)

        self._ready.set_result(all(results))
        self._done = True

    async def wait(self):
        ret = False
        try:
            ret = await asyncio.wait_for(self._ready, self._context.timeout)
        except asyncio.TimeoutError:
            self._context.log.warning('check timed out')
            self._ready.set_result(False)
            self._done = True
        return ret


@action('proxy.aggregate_replies')
class ProxyAggregateReplies(ActionBase):
    MSGTYPE_UNDEFINED = 0
    MSGTYPE_DIRECTIVE = 1
    MSGTYPE_STREAMCONTROL = 2

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__handlers = {}

    async def execute(self, context: Context):
        if context.id:
            context.message_id = context.dispacher.get_message_id(context.id)
        elif context.message_id is None:
            context.message_id = context['session'].last_message_id

        if context.message_id is None:
            context.report.ok = False
            context.report.message = 'no message id to wait'
            context.log.error(context.report.message)
            return False

        for message in context.messages:
            ctx = context('msg', tag='msg', data=message)
            ctx.timeout = context.get('timeout', 10.0)
            self.__handlers[ctx.message.lower()] = ProxyReplyHandler(ctx)

        context.log.debug('waiting for a messages %s', context.message_id)
        await context.dispatcher.wait_for_message(self, message_id=context.message_id)

        context.log.debug('no messages, still waiting for a messages %s', context.message_id)
        return await self.wait(context)

    async def on_message(self, message):
        message_type = ProxyAggregateReplies.MSGTYPE_UNDEFINED
        if 'streamcontrol' in message:
            message_type = ProxyAggregateReplies.MSGTYPE_STREAMCONTROL
        elif 'directive' in message:
            message_type = ProxyAggregateReplies.MSGTYPE_DIRECTIVE
            message_namespace = message.get('directive', {}).get('header', {}).get('namespace', 'Ns')
            message_name = message.get('directive', {}).get('header', {}).get('name', 'Nm')
            message_full_name = f'{message_namespace}.{message_name}'.lower()

            handler = self.__handlers.get(message_full_name)
            if handler is not None:
                handler._context.wsmsg = message
                handler._context.wsmsg_type = message_type
                await handler()

    async def on_close(self):
        self._context.report.ok = False
        self._context.report.message = 'waiting for message=%s failed: connection closing' % (
            self._context.message_id
        )
        self._context.log.error(self._context.report.message)
        self._context.reply.set_result(False)

    async def wait(self, context: Context):
        ret = False

        futures = [handler.wait() for k, handler in self.__handlers.items()]

        try:
            results = await asyncio.wait_for(asyncio.gather(*futures), context.get('timeout', 12.0))
            self._context.log.debug(results)
        except asyncio.TimeoutError as ex:
            context.report.ok = False
            context.report.message = f'waiting for message_id={context.message_id} timed out: {ex}'
            context.log.error(context.report.message)
            return False
        except Exception as ex:
            context.report.ok = False
            context.report.message = f'waiting for message_id={context.message_id} failed: {ex}'
            context.log.error(context.report.message)
            return False
        finally:
            await context.dispatcher.remove_wait(message_id=context.message_id)

        return ret
