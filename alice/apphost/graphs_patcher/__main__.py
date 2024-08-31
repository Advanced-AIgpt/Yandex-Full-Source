import argparse
import copy
import json
import os
import sys

import alice.apphost.library.graph_filters as graph_filters


FILTER_FUNCTIONS = {
    "all_jsons": graph_filters.leave_only_json_files,
    "all_graphs": graph_filters.leave_only_graph_files,
    "all_generated_megamind_scenarios_graphs": graph_filters.leave_only_generated_megamind_scenarios_graphs_files,
    "all_scenarios": graph_filters.leave_only_scenario_graphs_files,
    "all_graphs_with_hollywood_backends": graph_filters.leave_only_graphs_with_hollywood_backends,
    "all_hollywood_scenarios": graph_filters.leave_only_hollywood_scenarios_files,
}


def get_graphs_dir(arcadia_root_path):
    return os.path.join(arcadia_root_path, "apphost/conf/verticals/ALICE")


def get_all_apphost_files():
    arcadia_root_path = os.path.realpath(os.path.join(os.path.dirname(sys.argv[0]), "../../.."))
    print("Arcadia path:", arcadia_root_path)
    graphs_dir = get_graphs_dir(arcadia_root_path)
    return list(map(lambda name: os.path.join(graphs_dir, name), os.listdir(graphs_dir)))


def patch_function(graph_name, graph):
    print(f"Trying to apply patcher to {graph_name}")

    # Write your code here!!!
    # For exmaple:
    # if len(graph_name) == 42:
    #     graph["comment"] = "blablabla"

    return graph


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--filter", choices=list(FILTER_FUNCTIONS.keys()), default=None,
                        help="Choose filter for files in apphost/conf/verticals/ALICE")
    parser.add_argument("--verbose", "-v", action="store_true", help="verbose logs")
    args = parser.parse_args()

    filter = FILTER_FUNCTIONS[args.filter]

    files = filter(get_all_apphost_files())

    for file in files:
        with open(file) as fin:
            content = json.load(fin)
        graph_name = os.path.split(file)[1]
        new_content = patch_function(graph_name, copy.deepcopy(content))
        if args.verbose:
            if content == new_content:
                print(f"No diff for graph {graph_name}")
            else:
                print(f"Patch function applied to graph {graph_name}")
        with open(file, "w") as fout:
            json.dump(new_content, fout, indent=4)
            fout.write("\n")


if __name__ == "__main__":
    sys.exit(main() or 0)
