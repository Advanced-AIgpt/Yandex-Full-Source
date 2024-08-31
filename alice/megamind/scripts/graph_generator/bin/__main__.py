import alice.megamind.scripts.graph_generator.library.generator as generator

import os
import sys


def main():
    print("\033[91m Warning: This generator is deprecated.\n Use /arcadia/alice/apphost/graph_generator \033[0m")
    arcadia_root_path = os.path.realpath(os.path.join(os.path.dirname(sys.argv[0]), "../../../../../"))
    print("Arcadia path:", arcadia_root_path)
    generator.create_mm_scenarios_graph("alice/megamind/configs", "apphost/conf/verticals/ALICE", arcadia_root_path)
    generator.create_mm_combinators_graph("alice/megamind/configs", "apphost/conf/verticals/ALICE", arcadia_root_path)


if __name__ == '__main__':
    sys.exit(main() or 0)
