from alice.cuttlefish.library.python.test_utils import checks, messages, autowait
import asyncio


@autowait
async def process_tts_generate_in_uniproxy(uniproxy, tts_speak=None, tts_generate=None, wait_nextchunk=False):
    event = await uniproxy.read()
    if tts_generate is not None:
        assert checks.match(event, tts_generate)

    ref_message_id = event["event"]["header"]["messageId"]
    stream_id = event["event"]["header"]["refStreamId"]

    if tts_speak is None:
        tts_speak = messages.TtsSpeak(ref_message_id=ref_message_id, stream_id=stream_id)

    await uniproxy.write(tts_speak)

    for x in (b"A", b"B", b"C", b"D", b"E"):
        await uniproxy.write(messages.BinaryChunk(stream_id=stream_id, content=x * 256))
        await asyncio.sleep(0.1)
        if wait_nextchunk:
            # don't send next chunk until User sends StreamControl with NEXT_CHUNK action
            assert checks.match(
                await uniproxy.read(timeout=3),
                {"streamcontrol": {"streamId": stream_id, "action": messages.StreamControlAction.NEXT_CHUNK}},
            )

    await uniproxy.write(messages.StreamControl(stream_id=stream_id, action=messages.StreamControlAction.CLOSE))
    await uniproxy.write(messages.EventProcessorFinishedFor(event))
