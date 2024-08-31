import argparse
import sys
import os
import datetime
import json
import codecs
import math
from fractions import Fraction


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--data', dest='data', help='urls_matching with data fields', required=True)
    parser.add_argument('-a', '--assignments', dest='assignments', help='Assignments from toloka', required=True)
    parser.add_argument('-o', '--output', dest='output', help='output table to save table with links', required=True)
    parser.add_argument('-s', '--stat', dest='stat_output', help='output table to save table with links', required=True)
    context = parser.parse_args(sys.argv[1:])

    assignments = json.loads(open(context.assignments).read())
    data = json.loads(open(context.data).read())
    for x in data:
        x.update({"bad_votes": 0, "ok_votes": 0})
    data = dict([(x["downloadUrl"], x) for x in data])

    for assignment in assignments:
        url = assignment["inputValues"]["audio"]
        vote = assignment["outputValues"]["output"]
        data[url]["shown_text"] = assignment["inputValues"]["text"]
        if vote == "OK":
            data[url]["ok_votes"] += 1
        elif vote == "BAD":
            data[url]["bad_votes"] += 1
        else:
            raise ValueError("Wrong format in toloka input")

    for _, x in data.items():
        x.pop("rnd1", None)
        x.pop("initialFileName", None)
        x.pop("mdsFileName", None)
        x["result"] = "UNKNOWN"

        if x["ok_votes"] > x["bad_votes"]:
            x["result"] = "OK"

        elif x["ok_votes"] < x["bad_votes"]:
            x["result"] = "BAD"
    with codecs.open(context.output, 'w', encoding="utf8") as g:
        s = json.dumps(list(data.values()), indent=4, ensure_ascii=False)
        g.write(s)

    stat = {"pser": (sum((x["result"] == "BAD") * x.get("sample_count", 1) for x in data.itervalues()) * 100.0 /
                     sum(x.get("sample_count", 1) for x in data.itervalues())),
            "fielddate": datetime.datetime.now().strftime('%Y-%m-%d')}
    with open(context.stat_output, 'w') as g:
        g.write(json.dumps(stat, indent=4, ensure_ascii=False))


if __name__ == '__main__':
    main()
