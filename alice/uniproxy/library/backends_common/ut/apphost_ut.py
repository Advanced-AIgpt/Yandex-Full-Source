from alice.uniproxy.library.backends_common.apphost import AppHostHttpClient, Request, ItemFormat
import logging
import common
import tornado.concurrent
import json


@common.run_async
async def test_apphost_for_synchronize_state():
    from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TConnectionInfo
    from alice.cuttlefish.library.protos.wsevent_pb2 import TWsEvent, TEventHeader

    request = Request(
        path="synchronize_state_2",
        guid="aaaaaaaa-bbbbbbbb-cccccccc-dddddddd",
        items={
            "session_context": TSessionContext(
                ConnectionInfo=TConnectionInfo(
                    IpAddress="188.65.247.92"
                )
            ),
            "ws_message": TWsEvent(
                Header=TEventHeader(
                    Namespace=TEventHeader.EMessageNamespace.SYSTEM,
                    Name=TEventHeader.EMessageName.SYNCHRONIZE_STATE,
                    MessageId="aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
                ),
                Text=json.dumps({
                    "event": {
                        "header": {
                            "namespace": "System",
                            "name": "SynchronizeState",
                            "messageId": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
                        },
                        "payload": {
                            "uuid": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
                        }
                    }
                })
            )
        }
    )

    loop = tornado.ioloop.IOLoop.current()
    finished = tornado.concurrent.Future()

    async def client(url):
        try:
            ah_client = AppHostHttpClient(url)
            resp = await ah_client.fetch(request, request_timeout=3)
            logging.debug(f"response: {resp}")

            items = [it for it in resp.get_items(item_type="ws_message", proto_type=TWsEvent)]
            assert len(items) == 1
            assert items[0].format == ItemFormat.PROTOBUF
            assert items[0].type == "ws_message"
            assert items[0].source == "CONVERT_OUT"
            ws_event = items[0].data
            assert ws_event.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
            assert ws_event.Header.Name == TEventHeader.EMessageName.EVENT_EXCEPTION
            assert ws_event.Header.MessageId == "fae6d17a-249a19c8-d3981889-a4904a1d"
            assert json.loads(ws_event.Text) == {
                "directive": {
                    "header": {
                        "namespace": "System",
                        "name": "EventException",
                        "messageId": "fae6d17a-249a19c8-d3981889-a4904a1d"
                    },
                    "payload": {
                        "error": {
                            "message": "Invalid auth_token",
                            "type": "Error"
                        }
                    }
                }
            }

            finished.set_result(None)
        except Exception as exc:
            finished.set_exception(exc)

    try:
        with common.QueuedTcpServer() as srv:
            loop.add_callback(client, url=f"http://localhost:{srv.port}")

            stream = await srv.pop_stream()
            request = await common.read_http_request(stream, with_body=True)
            logging.debug(f"raw request (hex): {request.body.hex()}")

            assert request.method == "POST"
            assert request.path == "/synchronize_state_2"
            assert request.body.hex() == (
                "0a2361616161616161612d62626262626262622d63636363636363632d646464646464646412370a"
                "04494e4954120f73657373696f6e5f636f6e746578741a1e021300000000000000f004705f7a0f0a"
                "0d3138382e36352e3234372e393212be010a04494e4954120a77735f6d6573736167651aa90102ed"
                "00000000000000b3705f0a2a080110011a24610100102d08000b0500040200f10612bc017b226576"
                "656e74223a207b226865616465720b00f1076e616d657370616365223a202253797374656d222c20"
                "1700021200e06e6368726f6e697a6553746174651c00906d657373616765496421000462000b7900"
                "010f00040200b0227d2c20227061796c6f613400507b22757569090014222100012e000b05000302"
                "005061227d7d7d321373796e6368726f6e697a655f73746174655f32"
            )

            await stream.write(
                b"HTTP/1.1 200 OK\r\n"
                b"Content-Length: 350\r\n"
                b"Connection: Keep-Alive\r\n"
                b"\r\n" +
                bytes.fromhex(  # it's real response from AppHost
                    "0aec010a0b434f4e564552545f4f5554120a77735f6d6573736167651ad00102ed00000000000000"
                    "ff73705f0a29080110021a2366616536643137612d32343961313963382d64333938313838392d61"
                    "3439303461316412bd017b22646972656374697665223a7b22686561646572223a7b226e616d6573"
                    "70616365223a2253797374656d222c226e616d65223a224576656e74457863657074696f6e222c22"
                    "6d6573736167654964223a22780010b0227d2c227061796c6f61647500416572726f7400034700f0"
                    "05223a22496e76616c696420617574685f746f6b656600317479707e009072726f72227d7d7d7d0a"
                    "460a065f53544154531a3c023100000000000000f0225b7b226475726174696f6e223a353634382c"
                    "2261646472657373223a223139322e3136382e312e323a3434303030227d5d8a012077736e6b2d64"
                    "65762d766d322e7361732e79702d632e79616e6465782e6e65749801cb1a"
                )
            )
    except:
        logging.exception("server failed")
    finally:
        await finished
