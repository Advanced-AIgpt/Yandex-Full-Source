PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/testing
    alice/cuttlefish/tests/common
    alice/cuttlefish/tests/resources
    alice/iot/bulbasaur/protos/apphost
    alice/library/client/protos
    alice/megamind/protos/common
    alice/megamind/protos/guest
    alice/megamind/protos/speechkit
    alice/memento/proto
    alice/protos/api/matrix
    alice/uniproxy/library/protos
    apphost/lib/grpc/protos
    apphost/lib/proto_answers
    contrib/python/asynctest
    contrib/python/pytest-asyncio
    contrib/python/tornado/tornado-6
    library/python/codecs
    library/python/resource
    voicetech/library/proto_api
    voicetech/library/settings_manager/proto
    mssngr/router/lib/protos/registry
)

DEPENDS(
    alice/cuttlefish/bin/cuttlefish
    voicetech/tools/evlogdump
    alice/rtlog/evlogdump
)

DATA(
    # test_data_for_merger/
    #     /background.pcm
    #     /main_audio.opus
    #     /output_chunk0
    #     /output_chunk1
    #     /output_chunk2
    sbr://2456441357
)

TEST_SRCS(
    common/__init__.py
    common/constants.py
    common/cuttlefish.py
    common/items.py
    common/utils.py
    test_any_input.py
    test_audio_separator.py
    test_basic.py
    test_bio_context_load.py
    test_bio_context_save.py
    test_context_load.py
    test_context_save.py
    test_exp_flags.py
    test_log_spotter.py
    # test_megamind.py
    test_misc.py
    test_raw_to_protobuf.py
    test_store_audio.py
    test_stream_converter.py
    test_synchronize_state.py
    test_synchronize_state_blackbox.py
    test_tts_aggregator.py
    test_tts_request_sender.py
    test_tts_splitter.py
)

SIZE(MEDIUM)

REQUIREMENTS(ram:9)

END()
