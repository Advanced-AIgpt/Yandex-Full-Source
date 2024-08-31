import logging
import heapq


def parse_tskv(line):
    res = {}
    for kv in line.split("\t"):
        if not kv:
            continue
        k, v = kv.split("=", maxsplit=1)
        res[k] = v
    return res


def heap_by(records, key):
    logging.debug(f"Aggregate results by '{key}'...")
    aggregated = {}
    for record in records:
        val = record.get(key)
        aggregated.setdefault(val, 0)
        aggregated[val] += 1

    logging.debug(f"Sort aggregated by {key}...")
    heap = []
    for k, v in aggregated.items():
        heapq.heappush(heap, (v, k))

    return heap


def create_top(lines, n=50):
    records = [parse_tskv(l) for l in lines]

    for k in ("ip", "X-UPRX-AUTH-TOKEN", "X-UPRX-UUID"):
        print(f"--- BY {k.upper()}")
        agg = heap_by(records, k)
        for count, val in heapq.nlargest(n, agg):
            print(f"{val}\t{count}")
