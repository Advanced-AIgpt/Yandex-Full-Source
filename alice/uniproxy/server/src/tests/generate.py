#!/usr/bin/env python
from tornado.websocket import websocket_connect
from tornado.ioloop import IOLoop
import json


fo = open("out.pcm", "wb")

socket = None


def onConnect(con):
    global socket

    if con.exception():
        fuckit(con.exception())
        return

    socket = con.result()
    socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "TTS",
                "name": "Generate",
                "messageId": "olololo",
            },
            "payload": {
                "text": "1 2 3",
                "voice": "omazh",
                "lang": "ru-RU",
                "platform": "test",
                "application": "test",
                "format": "OPUS",
                "quality": "Ultrahigh",
                "key": "developers-simple-key",
            }
        }
    }))


def fuckit(msg):
    print(msg)
    IOLoop.current().stop()


def writeData(data):
    fo.write(data)


def onMessage(msg):
    global socket

    if msg is None:
        fuckit("Socket closed unexpected")
        return

    if len(msg) < 4:
        fuckit("Something nasty")
        return
    elif not isinstance(msg, bytes):
        try:
            res = json.loads(msg)
            if "streamcontrol" in res:
                fo.close()
                socket.write_message(
                    json.dumps({
                        "event": {
                            "header": {
                                "namespace": "TTS",
                                "name": "SpeechFinished",
                                "messageId": "kurwa"
                            },
                            "payload": {
                                "token": "WTF?"
                            }
                        }
                    })
                )
                fuckit("Done")
                return
            elif not "directive" in res:
                fuckit("Got not a directive: %s" % (msg,))
            directive = res.get("directive").get("header", {}).get("namespace", "") + "." + \
                res.get("directive").get("header", {}).get("name", "")
            if directive != "TTS.Speak":
                fuckit("Wrong directive: %s" % (directive,))
            else:
                print("Got TTS.Speak directive with stream=%s" % (res.get("directive").get("header").get("streamId"),))

                socket.write_message(json.dumps({
                    "event": {
                        "header": {
                            "namespace": "TTS",
                            "name": "SpeechStarted",
                            "messageId": "azaza"
                        },
                        "payload": {
                            "token": "WTF?"
                        }
                    }
                }))

            return
        except Exception as e:
            pass
    writeData(msg[4:])


websocket_connect("ws://localhost:8887/uni.ws", callback=onConnect, on_message_callback=onMessage)
IOLoop.current().start()
