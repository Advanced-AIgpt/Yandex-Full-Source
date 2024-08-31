import pytest
from asyncio.exceptions import TimeoutError
from .common import Cuttlefish

from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles

from alice.cuttlefish.library.protos.asr_pb2 import TAsrFinished
from alice.cuttlefish.library.protos.audio_pb2 import (
    TAudio,
    TAudioChunk,
    TBeginStream,
    TEndStream,
    TBeginSpotter,
    TEndSpotter,
    TMetaInfoOnly,
)
from alice.cuttlefish.library.protos.audio_separator_pb2 import TFullIncomingAudio
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TRequestContext


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def _get_begin_stream(mime="audio/opus"):
    return TAudio(
        BeginStream=TBeginStream(
            Mime=mime,
        ),
    )


def _get_audio_chunk(audio):
    return TAudio(
        Chunk=TAudioChunk(
            Data=audio,
        ),
    )


def _get_end_stream():
    return TAudio(
        EndStream=TEndStream(),
    )


def _get_begin_spotter():
    return TAudio(
        BeginSpotter=TBeginSpotter(),
    )


def _get_end_spotter():
    return TAudio(
        EndSpotter=TEndSpotter(),
    )


def _get_meta_info_only():
    return TAudio(
        MetaInfoOnly=TMetaInfoOnly(),
    )


def _check_response(
    response,
    expected_spotter_audio_part=None,
    expected_main_audio_part=None,
    expected_error_message=None,
):
    assert len(list(response.get_items())) == 1

    full_incoming_audio = response.get_only_item_data(
        item_type=ItemTypes.FULL_INCOMING_AUDIO, proto_type=TFullIncomingAudio
    )

    if expected_spotter_audio_part is not None:
        assert full_incoming_audio.SpotterPart == expected_spotter_audio_part
    else:
        assert not full_incoming_audio.HasField("SpotterPart")

    if expected_main_audio_part is not None:
        assert full_incoming_audio.MainPart == expected_main_audio_part
    else:
        assert not full_incoming_audio.HasField("MainPart")

    if expected_error_message is not None:
        assert full_incoming_audio.ErrorMessage == expected_error_message
    else:
        assert not full_incoming_audio.HasField("ErrorMessage")


def _send_items_chunked(stream, items, chunk_size):
    if chunk_size is None:
        chunk_size = len(items)

    chunks = []
    for i in range(0, len(items), chunk_size):
        current_chunk = dict()
        for j in range(i, min(i + chunk_size, len(items))):
            item_type, item_data = items[j]
            current_chunk.setdefault(item_type, []).append(item_data)

        chunks.append(current_chunk)

    for chunk in chunks:
        stream.write_items(chunk)


async def _check_no_output(stream, error_message):
    try:
        await stream.read(timeout=1.0)
        assert False, error_message
    except TimeoutError:
        pass


# -------------------------------------------------------------------------------------------------
class TestAudioSeparator:
    @pytest.mark.asyncio
    @pytest.mark.parametrize('with_spotter_part', [False, True])
    @pytest.mark.parametrize('items_in_one_apphost_chunk', [None, 3, 1])
    async def test_simple(
        self,
        cuttlefish: Cuttlefish,
        with_spotter_part,
        items_in_one_apphost_chunk,
    ):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.AUDIO_SEPARATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but audio separator return something")

            audio = [_get_begin_stream()]
            if with_spotter_part:
                audio += [
                    _get_begin_spotter(),
                    _get_audio_chunk(
                        audio=b"spotter_audio_chunk0",
                    ),
                    _get_audio_chunk(
                        audio=b"spotter_audio_chunk1",
                    ),
                    _get_end_spotter(),
                ]
            audio += [
                _get_audio_chunk(
                    audio=b"main_audio_chunk0",
                ),
                _get_audio_chunk(
                    audio=b"main_audio_chunk1",
                ),
                _get_end_stream(),
            ]

            _send_items_chunked(
                stream, [(ItemTypes.AUDIO, audio_item) for audio_item in audio], items_in_one_apphost_chunk
            )
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_spotter_audio_part=(
                    b"spotter_audio_chunk0spotter_audio_chunk1" if with_spotter_part else None
                ),
                expected_main_audio_part=b"main_audio_chunk0main_audio_chunk1",
                expected_error_message=None,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize('with_spotter_part', [False, True])
    @pytest.mark.parametrize('with_main_part', [False, True])
    @pytest.mark.parametrize('items_in_one_apphost_chunk', [None, 3, 1])
    async def test_with_asr_finished(
        self,
        cuttlefish: Cuttlefish,
        with_spotter_part,
        with_main_part,
        items_in_one_apphost_chunk,
    ):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.AUDIO_SEPARATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but audio separator return something")

            audio = [_get_begin_stream()]
            if with_spotter_part:
                audio += [
                    _get_begin_spotter(),
                    _get_audio_chunk(
                        audio=b"spotter_audio_chunk0",
                    ),
                    _get_audio_chunk(
                        audio=b"spotter_audio_chunk1",
                    ),
                ]

            if with_main_part:
                if with_spotter_part:
                    audio += [_get_end_spotter()]

                audio += [
                    _get_audio_chunk(
                        audio=b"main_audio_chunk0",
                    ),
                    _get_audio_chunk(
                        audio=b"main_audio_chunk1",
                    ),
                ]

            _send_items_chunked(
                stream,
                [(ItemTypes.AUDIO, audio_item) for audio_item in audio] + [(ItemTypes.ASR_FINISHED, TAsrFinished())],
                items_in_one_apphost_chunk,
            )

            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_spotter_audio_part=(
                    b"spotter_audio_chunk0spotter_audio_chunk1" if with_spotter_part else None
                ),
                expected_main_audio_part=(b"main_audio_chunk0main_audio_chunk1" if with_main_part else None),
                expected_error_message=None,
            )

    @pytest.mark.asyncio
    async def test_with_simple_error(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.AUDIO_SEPARATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(),
                    ],
                }
            )

            # Report error to other nodes (always response with full_incoming_audio)
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_spotter_audio_part=None,
                expected_main_audio_part=None,
                expected_error_message="Request context not found in first chunk",
            )

            # Report error to default apphost metrics
            response = await stream.read(timeout=1.0)
            assert response.has_exception() and b"Request context not found in first chunk" in response.get_exception()
