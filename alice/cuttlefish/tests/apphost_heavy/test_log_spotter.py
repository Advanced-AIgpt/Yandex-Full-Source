import pytest
import asyncio
from alice.cuttlefish.library.python.test_utils import messages, checks
from .common import initialized_connection


@pytest.mark.asyncio
async def test_log_spotter_diff(mocks, uniproxy2, uniproxy_mock):
    ORIG_EVENT = messages.LogSpotter(stream_id=1)
    STREAM_CONTROL = messages.StreamControl(stream_id=1)
    BINARY = [
        messages.BinaryChunk(1, bytes.fromhex("BABABABA")),
        messages.BinaryChunk(1, bytes.fromhex("CACACACA")),
        messages.BinaryChunk(1, bytes.fromhex("DADADADA")),
        messages.BinaryChunk(1, bytes.fromhex("FAFAFAFA")),
    ]

    async with uniproxy2.settings_patch(log_spotter_mode="ApphostDiff"):
        async with initialized_connection(uniproxy2, uniproxy_mock) as (user, uniproxy):
            await user.write(ORIG_EVENT)
            for b in BINARY:
                await user.write(b)
                await asyncio.sleep(0.02)
            await user.write(STREAM_CONTROL)

            msgs = await uniproxy.read_some()
            assert checks.match(msgs, [ORIG_EVENT, *BINARY, STREAM_CONTROL])
            await uniproxy.write(messages.EventProcessorFinishedFor(ORIG_EVENT))

            # check that AppHost graph has finished
            await asyncio.sleep(1.0)
            assert checks.match(await uniproxy2.get_unistat(), {"ah_stream_count_ammx": 0})

    assert checks.match(
        mocks.records,
        {
            "VOICE__MDS_STORE_HTTP": checks.ListMatcher(
                [{"request": {"method": "POST", "uri": checks.StartsWith("/upload-speechbase/test_")}}],
                exact_length=True,
            )
        },
    )


@pytest.mark.asyncio
async def test_log_spotter_apply(mocks, uniproxy2, uniproxy_mock):
    ORIG_EVENT = messages.LogSpotter(stream_id=1)
    STREAM_CONTROL = messages.StreamControl(stream_id=1)
    BINARY = [
        messages.BinaryChunk(1, bytes.fromhex("BABABABA")),
        messages.BinaryChunk(1, bytes.fromhex("CACACACA")),
        messages.BinaryChunk(1, bytes.fromhex("DADADADA")),
        messages.BinaryChunk(1, bytes.fromhex("FAFAFAFA")),
    ]

    async with uniproxy2.settings_patch(log_spotter_mode="ApphostApply"):
        async with initialized_connection(uniproxy2, uniproxy_mock) as (user, uniproxy):
            await user.write(ORIG_EVENT)
            for b in BINARY:
                await user.write(b)
                await asyncio.sleep(0.02)
            await user.write(STREAM_CONTROL)

            msg = await user.read()
            assert checks.match(
                msg,
                {
                    'directive': {
                        'header': {
                            'namespace': 'Log',
                            'name': 'Ack',
                            'refMessageId': ORIG_EVENT['event']['header']['messageId'],
                        },
                    },
                },
            )

            # check that AppHost graph has finished
            await asyncio.sleep(1.0)
            assert checks.match(await uniproxy2.get_unistat(), {"ah_stream_count_ammx": 0})
