import argparse

from alice.cuttlefish.tools.tts.graph_generator.library.graph_generators import (
    get_s3_audio_graph,
    get_tts_backend_graph,
    get_main_tts_graph,
    get_tts_generate_graph,
    graph_to_json_str,
)


def print_graph(graph_name, graph):
    with open(f"{graph_name}.json", "w") as f:
        f.write(graph_to_json_str(graph))


def main(arguments):
    print_graph("tts_backend", get_tts_backend_graph())
    print_graph("s3_audio", get_s3_audio_graph(arguments.s3_audio_http_node_count))
    print_graph("tts", get_main_tts_graph(arguments.s3_audio_http_node_count))
    print_graph("tts_generate", get_tts_generate_graph())


def parse_arguments():
    parser = argparse.ArgumentParser(description="Generate tts http graph.")
    parser.add_argument(
        "--s3-audio-http-node-count",
        dest="s3_audio_http_node_count",
        type=int,
        default=32,
        help="S3 audio http node count.",
    )

    return parser.parse_args()


if __name__ == "__main__":
    main(parse_arguments())
