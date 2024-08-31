import copy
import json

MONITORING_TEMPLATE = [{"operation": "perc", "prior": 101, "type": "failures", "warn": 0.25, "crit": 0.5}]

RESPONSIBLES_TEMPLATE = {
    "abc_service": [
        {
            "duty_slugs": [
                # In fact release is main duty slug
                # https://abc.yandex-team.ru/services/speechkit_ops/duty/?role=835
                # Proof: https://abc-back.yandex-team.ru/api/v4/duty/schedules/?service__slug=speechkit_ops
                "voiceinfra_duty_release"
            ],
            "slug": "speechkit_ops",
        }
    ],
    "logins": [
        # Personal notifications for feature owner
        "chegoryu"
        # TODO add alice duty chat
    ],
}

GRAPH_TEMPLATE = {
    "settings": {
        "input_deps": ["INIT"],
        "output_deps": ["RESPONSE"],
        "streaming_no_block_outputs": True,
        "node_deps": {"RESPONSE": {"input_deps": []}},
        "nodes": {},
        "responsibles": copy.deepcopy(RESPONSIBLES_TEMPLATE),
    }
}

COMMON_CPU_VOICES = [
    "cpu_alyss",
    "cpu_anton_samokhvalov",
    "cpu_dude",
    "cpu_erkanyavas",
    "cpu_ermil",
    "cpu_ermil_with_tuning",
    "cpu_ermilov",
    "cpu_good_oksana",
    "cpu_jane",
    "cpu_kolya",
    "cpu_kostya",
    "cpu_levitan",
    "cpu_nastya",
    "cpu_nick",
    "cpu_oksana",
    "cpu_omazh",
    "cpu_robot",
    "cpu_sasha",
    "cpu_silaerkan",
    "cpu_smoky",
    "cpu_tanya",
    "cpu_tatyana_abramova",
    "cpu_voicesearch",
    "cpu_zahar",
    "cpu_zhenya",
    "cpu_zombie",
]

TTS_BACKENDS = {
    "TTS_RU_GPU_SHITOVA": [
        "ru_gpu_shitova.gpu",
    ],
    "TTS_RU_GPU_SHITOVA_WHISPER": [
        "ru_gpu_shitova_whisper.gpu",
    ],
    "TTS_RU_GPU_STORYTELLER": [
        "ru_gpu_fairy_tales",
    ],
    "TTS_CLOUD_SYNTH": [
        "ru_gpu_vtb_brand_voice.cloud",
        "cloud_synth",
    ],
    "TTS_RU_GPU_OKSANA": [
        "ru_gpu_oksana_anton_samokhvalov.gpu",
        "ru_gpu_oksana_ermil.gpu",
        "ru_gpu_oksana_jane.gpu",
        "ru_gpu_oksana_kolya.gpu",
        "ru_gpu_oksana_kostya.gpu",
        "ru_gpu_oksana_krosh.gpu",
        "ru_gpu_oksana_nastya.gpu",
        "ru_gpu_oksana_oksana.gpu",
        "ru_gpu_oksana_omazh.gpu",
        "ru_gpu_oksana_sasha.gpu",
        "ru_gpu_oksana_tatyana_abramova.gpu",
        "ru_gpu_oksana_zahar.gpu",
    ],
    "TTS_RU_GPU_VALTZ": [
        "ru_gpu_valtz_valtz.gpu",
    ],
    "TTS_TR_GPU_SELAY": [
        "tr_gpu_selay.gpu",
    ],
    "TTS_EN_GPU": [
        "en_gpu_lj.gpu",
        "en_gpu_david.gpu",
    ],
    # Cpu legacy speakers
    "TTS_EN_CPU": [f"en_{voice}" for voice in COMMON_CPU_VOICES]
    + [
        "en_cpu_oksana.en",
    ],
    "TTS_RU_CPU": [f"ru_{voice}" for voice in COMMON_CPU_VOICES]
    + [
        "ru_cpu_assistant",
        "ru_cpu_fallback2jane",
        "ru_cpu_krosh",
        "ru_cpu_shitova",
        "ru_cpu_shitova.us",
        "ru_cpu_valtz",
        "ru_cpu_valtz.us",
    ],
    "TTS_TR_CPU": [f"tr_{voice}" for voice in COMMON_CPU_VOICES],
    "TTS_UK_CPU": [f"uk_{voice}" for voice in COMMON_CPU_VOICES],
    "TTS_AR_GPU_ARABIC": ["ar_gpu_arabic.gpu"],
}

S3_AUDIO_HTTP_REQUEST_PREFIX = "s3_audio_http_request_"
S3_AUDIO_HTTP_RESPONSE_PREFIX = "s3_audio_http_response_"
S3_AUDIO_BACKGROUND_NODE_NAME = "BACKGROUND"

TTS_BACKEND_REQUEST_PREFIX = "tts_backend_request_"


S3_NODE_TIMEOUT = "15s"
S3_SUBGRAPH_TIMEOUT = "15s"
S3_SUBGRAPH_CHUNK_WAIT_TIMEOUT = S3_SUBGRAPH_TIMEOUT

DEFAULT_STREAM_NODE_TIMEOUT = "60s"
DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT = "15s"
# Cache set returns nothing
# So we must have chunk_wait_timeout = timeout
TTS_CACHE_SET_CHUNK_WAIT_TIMEOUT = DEFAULT_STREAM_NODE_TIMEOUT


def _get_items_with_prefix(prefix, generator):
    return [f"{prefix}{item}" for item in generator]


def _items_to_str(items):
    return ",".join(items)


def _get_tts_backend_request_items(tts_backend):
    return _get_items_with_prefix(TTS_BACKEND_REQUEST_PREFIX, TTS_BACKENDS[tts_backend])


def _get_s3_audio_node_names(s3_audio_http_node_count):
    return list(map(str, list(range(s3_audio_http_node_count)) + [S3_AUDIO_BACKGROUND_NODE_NAME]))


def _get_s3_audio_http_request_items(s3_audio_http_node_count):
    return _get_items_with_prefix(
        S3_AUDIO_HTTP_REQUEST_PREFIX,
        map(lambda node_name: node_name.lower(), _get_s3_audio_node_names(s3_audio_http_node_count)),
    )


def _get_s3_audio_http_response_items(s3_audio_http_node_count):
    return _get_items_with_prefix(
        S3_AUDIO_HTTP_RESPONSE_PREFIX,
        map(lambda node_name: node_name.lower(), _get_s3_audio_node_names(s3_audio_http_node_count)),
    )


def _create_default_node(
    backend_name,
    timeout,
    chunk_wait_timeout=None,
    handler=None,
    never_discard=None,
    addr_aliases=None,
):
    node = {
        "backend_name": backend_name,
        "node_type": "DEFAULT",
        "monitoring": copy.deepcopy(MONITORING_TEMPLATE),
        "params": {"responsibles": copy.deepcopy(RESPONSIBLES_TEMPLATE), "timeout": timeout},
    }

    if chunk_wait_timeout is not None:
        node["params"]["chunk_wait_timeout"] = chunk_wait_timeout

    if handler is not None:
        node["params"]["handler"] = handler

    if never_discard is not None:
        node["never_discard"] = never_discard

    if addr_aliases is not None:
        node.setdefault("alias_config", dict())["addr_alias"] = addr_aliases

    return node


def get_all_tts_backend_request_items():
    result = []
    for tts_backend in sorted(TTS_BACKENDS.keys()):
        result.extend(_get_tts_backend_request_items(tts_backend))
    return result


def get_s3_audio_graph(s3_audio_http_node_count):
    graph = copy.deepcopy(GRAPH_TEMPLATE)

    for node_name in _get_s3_audio_node_names(s3_audio_http_node_count):
        graph["settings"]["nodes"][f"S3_AUDIO_{node_name}"] = _create_default_node(
            "VOICE__S3",
            S3_NODE_TIMEOUT,
            addr_aliases=["S3_AUDIO_ALL"],
        )
        graph["settings"]["node_deps"][f"S3_AUDIO_{node_name}"] = {
            "input_deps": [f"INIT@{S3_AUDIO_HTTP_REQUEST_PREFIX}{node_name.lower()}->http_request"]
        }
        graph["settings"]["node_deps"]["RESPONSE"]["input_deps"].append(
            f"S3_AUDIO_{node_name}@http_response->{S3_AUDIO_HTTP_RESPONSE_PREFIX}{node_name.lower()}"
        )

    return graph


def get_tts_backend_graph():
    graph = copy.deepcopy(GRAPH_TEMPLATE)

    response_input_deps = []
    for tts_backend in sorted(TTS_BACKENDS.keys()):
        graph["settings"]["nodes"][tts_backend] = _create_default_node(
            f"VOICE__{tts_backend}",
            DEFAULT_STREAM_NODE_TIMEOUT,
            chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
            handler="/tts",
            addr_aliases=["TTS_BACKENDS_ALL"],
        )
        graph["settings"]["node_deps"][tts_backend] = {
            "input_deps": [f"INIT@{_items_to_str(_get_tts_backend_request_items(tts_backend))}"]
        }
        response_input_deps.append(f"{tts_backend}@audio")

    graph["settings"]["node_deps"]["RESPONSE"] = {"input_deps": response_input_deps}

    return graph


def get_main_tts_graph(s3_audio_http_node_count):
    graph = copy.deepcopy(GRAPH_TEMPLATE)

    graph["settings"]["nodes"]["REQUEST_CONTEXT_SYNC"] = {"node_type": "TRANSPARENT"}
    graph["settings"]["node_deps"]["REQUEST_CONTEXT_SYNC"] = {"input_deps": ["INIT@^request_context,^session_context"]}

    graph["settings"]["nodes"]["SPLITTER"] = _create_default_node(
        "VOICE__CUTTLEFISH_BIDIRECTIONAL",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/tts_splitter",
    )
    graph["settings"]["node_deps"]["SPLITTER"] = {
        "input_deps": ["INIT@tts_partial_request,tts_request", "!REQUEST_CONTEXT_SYNC@request_context,session_context"]
    }

    graph["settings"]["nodes"]["TTS_CACHE_GET"] = _create_default_node(
        "VOICE__TTS_CACHE_PROXY",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/tts_cache",
    )
    graph["settings"]["node_deps"]["TTS_CACHE_GET"] = {
        "input_deps": ["SPLITTER@tts_cache_get_request,tts_cache_warm_up_request"]
    }

    graph["settings"]["nodes"]["TTS_REQUEST_SENDER_REQUEST_SYNC"] = {"node_type": "TRANSPARENT"}
    graph["settings"]["node_deps"]["TTS_REQUEST_SENDER_REQUEST_SYNC"] = {
        "input_deps": ["SPLITTER@^tts_request_sender_request"]
    }

    graph["settings"]["nodes"]["REQUEST_SENDER"] = _create_default_node(
        "VOICE__CUTTLEFISH_BIDIRECTIONAL",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/tts_request_sender",
    )
    graph["settings"]["node_deps"]["REQUEST_SENDER"] = {
        "input_deps": [
            "!TTS_REQUEST_SENDER_REQUEST_SYNC@tts_request_sender_request",
            "TTS_CACHE_GET@tts_cache_get_response_status",
        ]
    }

    graph["settings"]["nodes"]["TTS_AGGREGATOR_REQUEST_SYNC"] = {"node_type": "TRANSPARENT"}
    graph["settings"]["node_deps"]["TTS_AGGREGATOR_REQUEST_SYNC"] = {"input_deps": ["SPLITTER@^tts_aggregator_request"]}

    graph["settings"]["nodes"]["AGGREGATOR"] = _create_default_node(
        "VOICE__CUTTLEFISH_BIDIRECTIONAL",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/tts_aggregator",
    )
    graph["settings"]["node_deps"]["AGGREGATOR"] = {
        "input_deps": [
            "TTS_BACKEND@audio",
            f"S3_AUDIO@{_items_to_str(_get_s3_audio_http_response_items(s3_audio_http_node_count))}",
            "TTS_CACHE_GET@tts_cache_get_response",
            "!TTS_AGGREGATOR_REQUEST_SYNC@tts_aggregator_request",
        ]
    }

    graph["settings"]["nodes"]["TTS_CACHE_SET"] = _create_default_node(
        "VOICE__TTS_CACHE_PROXY",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=TTS_CACHE_SET_CHUNK_WAIT_TIMEOUT,
        handler="/tts_cache",
        never_discard=True,
    )
    graph["settings"]["node_deps"]["TTS_CACHE_SET"] = {"input_deps": ["AGGREGATOR@tts_cache_set_request"]}

    graph["settings"]["nodes"]["TTS_BACKEND"] = _create_default_node(
        "GRPC_SELF__VOICE__child_2",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/_streaming_no_block_outputs/_subhost/tts_backend",
    )
    graph["settings"]["node_deps"]["TTS_BACKEND"] = {
        "input_deps": [f"REQUEST_SENDER->INIT@{_items_to_str(get_all_tts_backend_request_items())}"]
    }

    graph["settings"]["nodes"]["S3_AUDIO"] = _create_default_node(
        "GRPC_SELF__VOICE__child_2",
        S3_SUBGRAPH_TIMEOUT,
        chunk_wait_timeout=S3_SUBGRAPH_CHUNK_WAIT_TIMEOUT,
        handler="/_streaming_no_block_outputs/_subhost/s3_audio",
    )
    graph["settings"]["node_deps"]["S3_AUDIO"] = {
        "input_deps": [f"SPLITTER->INIT@{_items_to_str(_get_s3_audio_http_request_items(s3_audio_http_node_count))}"]
    }

    graph["settings"]["node_deps"]["RESPONSE"] = {"input_deps": ["AGGREGATOR@audio,tts_timings"]}

    return graph


def get_tts_generate_graph():
    graph = copy.deepcopy(GRAPH_TEMPLATE)

    graph["settings"]["nodes"]["WS_ADAPTER_IN"] = _create_default_node(
        "VOICE__CUTTLEFISH_BIDIRECTIONAL",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/stream_raw_to_protobuf",
    )
    graph["settings"]["node_deps"]["WS_ADAPTER_IN"] = {
        "input_deps": ["INIT@session_context,settings_from_manager,ws_message"]
    }

    graph["settings"]["nodes"]["TTS"] = _create_default_node(
        "GRPC_SELF__VOICE__child_2",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/_streaming_no_block_outputs/_subhost/tts",
    )
    graph["settings"]["node_deps"]["TTS"] = {
        "input_deps": ["INIT@session_context", "WS_ADAPTER_IN->INIT@request_context,tts_request,tts_partial_request"]
    }

    graph["settings"]["nodes"]["WS_ADAPTER_OUT"] = _create_default_node(
        "VOICE__CUTTLEFISH_BIDIRECTIONAL",
        DEFAULT_STREAM_NODE_TIMEOUT,
        chunk_wait_timeout=DEFAULT_STREAM_NODE_CHUNK_WAIT_TIMEOUT,
        handler="/stream_protobuf_to_raw",
    )
    graph["settings"]["node_deps"]["WS_ADAPTER_OUT"] = {
        "input_deps": [
            "INIT@session_context,directive",
            "WS_ADAPTER_IN@request_context,directive",
            "TTS@audio,tts_timings",
        ]
    }

    graph["settings"]["node_deps"]["RESPONSE"] = {
        "input_deps": ["TTS@uniproxy2_directive", "WS_ADAPTER_OUT@ws_message,uniproxy2_directive"]
    }

    return graph


def graph_to_json_str(graph):
    return json.dumps(graph, indent=4, sort_keys=True) + "\n"
