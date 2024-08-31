from uuid import uuid4
import asyncio
from .misc import deepupdate
import struct


# -------------------------------------------------------------------------------------------------
class StreamControlAction:
    CLOSE = 0
    CHUNK = 1
    SPOTTER_END = 2
    NEXT_CHUNK = 3


# -------------------------------------------------------------------------------------------------
def SynchronizeState(message_id=None, payload=None):
    mvp = {"uuid": str(uuid4()), "auth_token": "069b6659-984b-4c5f-880e-aaedcfd84102"}  # this token is in whitelist

    return {
        "event": {
            "header": {"namespace": "System", "name": "SynchronizeState", "messageId": message_id or str(uuid4())},
            "payload": mvp if (payload is None) else deepupdate(mvp, payload),
        }
    }


# -------------------------------------------------------------------------------------------------
def LogSpotter(message_id=None, stream_id=1, payload=None):
    msg = {
        "event": {
            "header": {
                "namespace": "Log",
                "name": "Spotter",
                "messageId": message_id or str(uuid4()),
                "streamId": stream_id,
            },
            "payload": {} if payload is None else payload,
        }
    }
    return msg


# -------------------------------------------------------------------------------------------------
def EventProcessorFinished(ref_message_id, full_name, message_id=None):
    msg = {
        "directive": {
            "header": {
                "namespace": "Uniproxy2",
                "name": "EventProcessorFinished",
                "refMessageId": ref_message_id,
                "messageId": message_id or str(uuid4()),
            },
            "payload": {"event_full_name": full_name.lower()},
        }
    }
    return msg


# -------------------------------------------------------------------------------------------------
def TextInput(message_id=None, payload=None):
    msg = {
        "event": {
            "header": {
                "namespace": "Vins",
                "name": "TextInput",
                "messageId": message_id or str(uuid4()),
            },
            "payload": payload or {},
        }
    }
    return msg


# -------------------------------------------------------------------------------------------------
def EventProcessorFinishedFor(event, message_id=None):
    hdr = event["event"]["header"]
    return EventProcessorFinished(
        ref_message_id=hdr["messageId"], full_name=hdr["namespace"] + "." + hdr["name"], message_id=message_id
    )


# -------------------------------------------------------------------------------------------------
def TtsSpeak(ref_message_id, stream_id=None, message_id=None, payload=None):
    msg = {
        "directive": {
            "header": {
                "namespace": "TTS",
                "name": "Speak",
                "refMessageId": ref_message_id,
                "messageId": message_id or str(uuid4()),
                "streamId": stream_id or 2,
            },
            "payload": {} if (payload is None) else payload,
        }
    }
    return msg


# -------------------------------------------------------------------------------------------------
def BinaryChunk(stream_id, content=b"A" * 256):
    return struct.pack(">I", stream_id) + content


# -------------------------------------------------------------------------------------------------
def StreamControl(message_id=None, stream_id=1, action=StreamControlAction.CLOSE, reason=0):
    msg = {
        "streamcontrol": {
            "messageId": message_id or str(uuid4()),
            "streamId": stream_id,
            "action": action,
            "reason": reason,
        }
    }
    return msg


# -------------------------------------------------------------------------------------------------
async def binary_stream(ws, stream_id=1, content=b"A" * 256, delay=0.02, count=10, stream_control=False):
    while count > 0:
        await ws.write_message(BinaryChunk(stream_id, content=content))
        count -= 1
        if delay is not None:
            await asyncio.sleep(delay)
    if stream_control:
        await ws.write_json(StreamControl(stream_id=stream_id))
