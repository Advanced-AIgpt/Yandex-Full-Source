import logging
import json
import os
from . import register_test, SECRET
from alice.cuttlefish.library.python.testing import items
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.checks import match
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TConnectionInfo, TUserInfo



@register_test
async def simple_synchronize_state(client):

    async with client.create_stream(path="synchronize_state_2") as stream:
        stream.write_items(
            items={
                ItemTypes.SESSION_CONTEXT: [
                    TSessionContext(
                        ConnectionInfo=TConnectionInfo(
                            IpAddress="188.65.247.92"
                        )
                    )
                ],
                ItemTypes.WS_MESSAGE: [
                    items.create_ws_message({
                        "event": {
                            "header": {
                                "namespace": "System",
                                "name": "SynchronizeState",
                                "messageId": "14"
                            },
                            "payload": {
                                "uuid": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                                "auth_token": "069b6659-984b-4c5f-880e-aaedcfd84102",  # is in whitelist, no request to APIKEYS
                                "oauth_token": SECRET["oauth_token"],  # 'yndx.quasar.test.1', uid = 653600380
                                "vins": {
                                    "application": {
                                        "app_id": "best.app.id.ever"
                                    }
                                }
                            }
                        }
                    })
                ]
            },
            last=True
        )

        resp = await stream.read()
        logging.debug(f"RESPONSE:\n{resp}")

        for it in resp.get_items():
            logging.debug(it)
