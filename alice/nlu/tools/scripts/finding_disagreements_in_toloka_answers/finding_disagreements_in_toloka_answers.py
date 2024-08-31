import argparse
import numpy as np
import pandas as pd
import collections

ID_LEN = 32


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-path", required=True, help="Input file (downloaded from Toloka, tsv) path")
    parser.add_argument("--output-path", required=True, help="Output file path")
    parser.add_argument("--input-column", default="query", help="Column suffix in input file, where query is stored")
    parser.add_argument("--output-column", default="result",
                        help="Column suffix in input file, where toloker's answer is stored")

    args = parser.parse_args()

    df = pd.read_csv(args.input_path, sep="\t")

    got_ans_for_q = collections.defaultdict(list)
    cut_params = [f"INPUT:{args.input_column}", f"OUTPUT:{args.output_column}",
                  "ASSIGNMENT:started",
                  "ASSIGNMENT:worker_id"]
    for _, (query, answer, time, id_) in df[cut_params].iterrows():
        if query is np.nan:
            continue
        if answer is np.nan:
            answer = None
        got_ans_for_q[query].append({"ans": answer, "time": time, "id": id_})

    with open(args.output_path, "w") as f:
        for query, answer_list in got_ans_for_q.items():
            if len(set(answer["ans"] for answer in answer_list if answer["ans"])) > 1:
                print(" " * ID_LEN, query, file=f)
                for answer in sorted(answer_list, key=lambda answer: answer["time"]):
                    print(answer["id"], answer["ans"], file=f)
