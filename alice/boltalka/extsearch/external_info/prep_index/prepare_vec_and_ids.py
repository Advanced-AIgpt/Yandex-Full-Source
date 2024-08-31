import argparse
import yt.wrapper as yt
import numpy as np
import tqdm

yt.config.set_proxy('hahn')
yt.config["read_parallel"]["enable"] = True
yt.config["read_parallel"]["max_thread_count"] = 20

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--in-table", required=True, type=str, help="YT table path to traverse.")
    parser.add_argument("--out-local-prefix", required=True, type=str)

    args = parser.parse_args()

    with open(args.out_local_prefix + '.vec', 'wb') as vecs, \
         open(args.out_local_prefix + '.ids', 'wb') as ids:
        for row in tqdm.tqdm(yt.read_table(args.in_table, format=yt.YsonFormat(encoding=None))):
            vecs.write(np.array(row[b'embedding'], dtype=np.float32).tobytes())
            doc_id = row[b'doc_id']
            ids.write(np.array([doc_id], dtype=np.uint32).tobytes())

if __name__ == '__main__':
    main()
