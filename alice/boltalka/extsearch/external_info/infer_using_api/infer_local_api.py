import argparse
from tqdm import tqdm
import requests

import yt.wrapper as yt


def infer_api(yt_in_table_path, yt_out_table_path, context_col, api_endpoint):
    headers = {"Content-Type": "application/json"}
    results = []
    for row in tqdm(yt.read_table(yt_in_table_path)):
        r = requests.post(url=api_endpoint, json={
            "Context": [x for x in reversed(row[context_col]) if x], "NumHypos": 1, "Seed": {"Value": 311}
        }, headers=headers)

        results.append({"reply": r.json()["Responses"][0]["Response"], "context": row[context_col]})

    yt.write_table(yt_out_table_path, results, format="json")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--yt-in-table-path", required=True, type=str)
    parser.add_argument("--yt-out-table-path", required=True, type=str)
    parser.add_argument("--context-col", required=True, type=str)
    parser.add_argument("--api-endpoint", required=True, type=str)
    args = parser.parse_args()
    infer_api(**vars(args))
