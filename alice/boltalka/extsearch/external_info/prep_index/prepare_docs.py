import argparse
from tqdm import tqdm
import json

import yt.wrapper as yt

yt.config.set_proxy('hahn')
yt.config["read_parallel"]["enable"] = True
yt.config["read_parallel"]["max_thread_count"] = 20

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--in-table", required=True, type=str, help="YT table path to traverse.")
    parser.add_argument("--out-local-file", required=True, type=str)
    args = parser.parse_args()

    data = [(int(row["doc_id"]), row["text"]) for row in tqdm(yt.read_table(args.in_table))]
    data = [text for doc_id, text in sorted(data)]
    with open(args.out_local_file, "w") as f:        
        f.write(json.dumps(data))
