from aiohttp import web
from typing import Any
from alice.paskills.penguinarium.util.metrics import sensors


@web.middleware
async def metrics_middleware(
    request: web.BaseRequest,
    handler: Any
) -> web.Response:
    try:
        sensors.set_empty_labels_stack()
        with sensors.labels_context({'path': request.path}):
            async with sensors.timer('http_response_time'):
                await sensors.inc_counter('http_requests')

                try:
                    resp = await handler(request)
                    await sensors.inc_counter(
                        'http_responses',
                        labels={'status_code': resp.status},
                    )
                except web.HTTPException as ex:
                    await sensors.inc_counter(
                        'http_responses',
                        labels={'status_code': ex.status},
                    )
                    raise
    finally:
        await sensors.storage.flush()

    return resp
