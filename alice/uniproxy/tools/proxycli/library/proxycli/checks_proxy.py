from .actions import *  # noqa
from .actions import ActionBase, action
from .context import Context


@action('proxy.check.event_name')
class ProxyCheckEventName(ActionBase):
    async def execute(self, context: Context):
        if context.namespace is None and context.name is None:
            context.report.ok = True
            context.report.message = 'any message is acceptable'
            context.log.warning(context.report.message)
            return True

        namespace = context.message.get('directive', {}).get('header', {}).get('namespace')
        name = context.message.get('directive', {}).get('header', {}).get('name')

        required_event_name = f'{context.namespace}.{context.name}'
        event_name = f'{namespace}.{name}'

        if event_name != required_event_name:
            context.report.ok = False
            context.report.message = f'expecting {required_event_name} got {event_name}'
            context.log.error(context.report.message)
            return False

        context.report.ok = True
        context.report.message = f'{event_name}'

        return True


@action('proxy.check.event_payload')
class ProxyCheckEventPayload(ActionBase):
    async def execute(self, context: Context):
        payload = context.message.get('directive', {}).get('payload', {})

        value = payload
        path = context.key.split('.')

        expected = context.value
        if isinstance(expected, str):
            if expected.startswith('%') and expected.endswith('%'):
                expected_name = expected[1:-1]
                expected = context.get(expected_name, expected)
            elif expected.startswith('$') and expected.endswith('$'):
                expected_name = expected[1:-1]
                expected = context.get(expected_name, expected)

        while value is not None and path:
            value = value.get(path[0])
            path = path[1:]

        if not path:
            if value == expected:
                context.report.ok = True
                context.report.message = f'{context.key} => {expected}'
                return True

        context.report.ok = False
        context.report.message = f'{context.key} => {value}, expected {expected}'
        context.log.debug(context.report.message)
        return False


@action('proxy.check.event_payload_one_of')
class ProxyCheckEventPayloadOneOf(ActionBase):
    async def execute(self, context: Context):
        payload = context.message.get('directive', {}).get('payload', {})

        value = payload
        path = context.key.split('.')

        while value is not None and path:
            value = value.get(path[0])
            path = path[1:]

        if not path:
            if value in context.values:
                context.report.ok = True
                context.report.message = f'{context.key} => {value}'
                return True

        context.report.ok = False
        context.report.message = f'{context.key} => {value}, expected one of {context.values}'
        context.log.warning(context.report.message)
        return False


@action('proxy.check.asr_normalized')
class ProxyCheckASRNormalized(ActionBase):
    async def execute_one_of(self, context, recognitions):
        results = []

        for recognition in recognitions:
            normalized = recognition.get('normalized')
            if normalized is None:
                continue

            if normalized in context.oneof:
                context.report.ok = True
                context.report.message = f'normalized: "{normalized}"'
                return True
            else:
                results.append(normalized)

        context.report.ok = False
        context.report.message = f'none of {results} was expected'
        return False

    async def execute_first(self, context, recognitions):
        if len(recognitions) < 1:
            context.report.ok = False
            context.report.message = 'no recognitions'
            return False

        normalized = recognitions[0].get('normalized')
        if normalized and normalized in context.first:
            context.report.ok = True
            context.report.message = f'normalized "{normalized}"'
            return True
        else:
            context.report.ok = False
            context.report.message = f'normalized "{normalized}" expecting one of {context.first}'
            return False

    async def execute(self, context: Context):
        recognitions = context.message.get('directive', {}).get('payload', {}).get('recognition', [])

        if context.oneof:
            return await self.execute_one_of(context, recognitions)
        elif context.first:
            return await self.execute_first(context, recognitions)
        else:
            context.report.skipped = True
            context.report.message = 'unknown mode of check operation'
            return True


@action('proxy.check.asr_response')
class ProxyCheckASRResponse(ActionBase):
    async def execute(self, context: Context):
        context.report.ok = True
        context.report.message = None
        context.report.skipped = True
        return True
