import logging

import tornado.httpclient
import tornado.gen

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest, HTTPError
from alice.uniproxy.library.global_counter import UnistatTiming, GlobalCounter
from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayMessage

from alice.uniproxy.library.subway.push_client.locations_remover import schedule_remove_guids
from alice.uniproxy.library.subway.push_client.device_location_remover import schedule_remove_devices


PUSH_MESSAGE_SUCCESS = (True, 'ok')


@tornado.gen.coroutine
def push_message(host: str, message: TSubwayMessage, port: int = 7777, log: logging.Logger = None, alias: str = None, device_id=None, remover_enabled=True):
    with UnistatTiming('mssngr_out_post'):
        try:
            request = HTTPRequest(
                '/push',
                method='POST',
                headers={
                    'Content-Type': 'application/octet-stream',
                },
                body=message.SerializeToString(),
                request_timeout=0.7,
                retries=2
            )

            client = QueuedHTTPClient.get_client(host, port, pool_size=15, queue_size=1000)

            response = yield client.fetch(request)

            GlobalCounter.DELIVERY_PUSH_200_SUMM.increment()

            if remover_enabled:
                remover = schedule_remove_guids
                if device_id:
                    remover = schedule_remove_devices

                if alias:
                    remover(response, alias)
                else:
                    remover(response, host)

        except HTTPError as ex:
            if log:
                log.error('PUSH %s %d: %s', host, ex.code, ex.body)

            if ex.code == 599:
                GlobalCounter.DELIVERY_PUSH_599_SUMM.increment()
            else:
                status = ex.code // 100
                if status == 4:
                    GlobalCounter.DELIVERY_PUSH_4XX_SUMM.increment()
                elif status == 5:
                    GlobalCounter.DELIVERY_PUSH_5XX_SUMM.increment()
                else:
                    GlobalCounter.DELIVERY_PUSH_OTHER_ERR_SUMM.increment()

            return False, str(ex)

        except Exception as ex:
            if log:
                log.error('PUSH Exception %s %s', host, str(ex))
            GlobalCounter.DELIVERY_PUSH_OTHER_ERR_SUMM.increment()

            return False, str(ex)

        return PUSH_MESSAGE_SUCCESS
