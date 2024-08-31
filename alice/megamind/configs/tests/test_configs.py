import alice.megamind.library.config.scenario_protos.config_pb2 as scenario_config
import alice.megamind.library.config.scenario_protos.combinator_config_pb2 as combinator_config
import google.protobuf.text_format as text
import yatest.common

import logging
import os


graphs_path = yatest.common.source_path("alice/megamind/configs")


def test_proto_parsing():
    for env in ["dev", "hamster", "production", "rc"]:
        for file in os.listdir(os.path.join(graphs_path, env, "scenarios")):
            if not file.endswith(".pb.txt"):
                continue
            in_str = open(os.path.join(graphs_path, env, "scenarios", file)).read()
            proto = scenario_config.TScenarioConfig()
            try:
                text.Parse(in_str, proto)
            except Exception as e:
                logging.error("Failed to parse " + os.path.join(env, file)+ ", error: " + str(e))
                raise e

        for file in os.listdir(os.path.join(graphs_path, env, "combinators")):
            if not file.endswith(".pb.txt"):
                continue
            in_str = open(os.path.join(graphs_path, env, "combinators", file)).read()
            proto = combinator_config.TCombinatorConfigProto()
            try:
                text.Parse(in_str, proto)
            except Exception as e:
                logging.error("Failed to parse " + os.path.join(env, file)+ ", error: " + str(e))
                raise e
