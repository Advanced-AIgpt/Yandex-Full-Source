import pytest
import yatest.common

from alice.cuttlefish.tools.tts.graph_generator.library.graph_generators import (
    get_all_tts_backend_request_items,
    get_s3_audio_graph,
    get_tts_backend_graph,
    get_main_tts_graph,
    get_tts_generate_graph,
    graph_to_json_str,
)

tts_graphs = {
    "tts_backend": [get_tts_backend_graph, []],
    "s3_audio": [get_s3_audio_graph, [32]],
    "tts": [get_main_tts_graph, [32]],
    "tts_generate": [get_tts_generate_graph, []],
}


@pytest.mark.parametrize("graph", sorted(tts_graphs.keys()))
def test_tts_graphs_in_sync(graph):
    generator, generator_args = tts_graphs[graph]

    graph_file_path = yatest.common.source_path(f"apphost/conf/verticals/VOICE/{graph}.json")

    with open(graph_file_path) as f:
        data = f.read()

    assert data == graph_to_json_str(generator(*generator_args)), f"graph: {graph}"


def test_all_tts_backend_request_items_has_unique_route():
    # Limit for line length is too small for python (only 200 characters)
    # so we need to separate this to four lines
    voicetech_only_tts_backend_request_item_types_canondata_dir = (
        "alice/cuttlefish/library/cuttlefish/tts/utils/tests_canonize/canondata/"
        + "exectest.run_TCuttlefishTtsUtilsCanonizeTest.TestGetTtsBackendRequestItemTypeForLang_"
    )
    voicetech_only_tts_backend_request_item_types_canondata_file = (
        "TCuttlefishTtsUtilsCanonizeTest.TestGetTtsBackendRequestItemTypeForLang.stdout"
    )
    voicetech_only_tts_backend_request_item_types_file_path = yatest.common.source_path(
        f"{voicetech_only_tts_backend_request_item_types_canondata_dir}/{voicetech_only_tts_backend_request_item_types_canondata_file}"
    )

    with open(voicetech_only_tts_backend_request_item_types_file_path) as f:
        all_possible_tts_backend_request_item_types_list = [line.rstrip() for line in f.readlines()]
        all_possible_tts_backend_request_item_types_list.extend(['tts_backend_request_cloud_synth'])

    all_possible_tts_backend_request_item_types_set = set()
    for tts_backend_request_item_type in all_possible_tts_backend_request_item_types_list:
        assert (
            tts_backend_request_item_type not in all_possible_tts_backend_request_item_types_set
        ), f"All item types should be unique, but {tts_backend_request_item_type} occur twice in canondata"
        all_possible_tts_backend_request_item_types_set.add(tts_backend_request_item_type)

    # Keep this set empty or ask chegoryu@ how to add something to it correctly
    bad_tts_backend_request_item_types = set()
    # Just sanity check
    assert (
        bad_tts_backend_request_item_types & all_possible_tts_backend_request_item_types_set
    ) == bad_tts_backend_request_item_types

    tts_backend_request_item_types_from_generator_list = get_all_tts_backend_request_items()
    tts_backend_request_item_types_from_generator_set = set()
    for tts_backend_request_item_type in tts_backend_request_item_types_from_generator_list:
        assert (
            tts_backend_request_item_type not in tts_backend_request_item_types_from_generator_set
        ), f"All item types should be unique, but {tts_backend_request_item_type} occur twice in generator data"
        assert (
            tts_backend_request_item_type in all_possible_tts_backend_request_item_types_set
        ), f"Item type from generator {tts_backend_request_item_type} missed in all possible tts backend request canondata"

        tts_backend_request_item_types_from_generator_set.add(tts_backend_request_item_type)

    for tts_backend_request_item_type in all_possible_tts_backend_request_item_types_list:
        if tts_backend_request_item_type in bad_tts_backend_request_item_types:
            assert (
                tts_backend_request_item_type not in tts_backend_request_item_types_from_generator_set
            ), f"Item type {tts_backend_request_item_type} present in generator data and in exceptions simultaneously"
        else:
            assert (
                tts_backend_request_item_type in tts_backend_request_item_types_from_generator_set
            ), f"Item type {tts_backend_request_item_type} missed in generator data"
