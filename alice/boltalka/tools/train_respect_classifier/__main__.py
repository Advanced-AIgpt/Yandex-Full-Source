# -*- coding: utf-8 -*-
import yt.wrapper as yt
from alice.boltalka.py_libs.respect_classifier import get_context_features


def process(reply):
    for i, token in enumerate(reply):
        if token in [u"вас", u"вам"]:
            features = get_context_features(reply, i)
            features["answer"] = token == u"вам"
            yield features


def mapper(row):
    reply = row["reply"].decode("utf-8")
    for el in process(reply):
        yield el


def main():
    import sys
    for line in sys.stdin:
        for el in process(line.strip().decode("utf-8")):
            print el


def yt_main():
    yt.config["proxy"]["url"] = "hahn.yt.yandex.net"
    yt.config['pickling']['dynamic_libraries']['enable_auto_collection'] = True
    src = (
        "//home/voice/alipov/build_index/alipov/nirvana/f77b1776-8c6e-4395-865e-6a024d76f9c3/"
        + "output__BBV1OcVqRWG96C2yc5dzgg/reply")
    dst = "//home/voice/nzinov/dataset"
    row_count = yt.get(input + '/@row_count')
    rows_per_job = 10000
    job_count = min((row_count + rows_per_job - 1) // rows_per_job, 1000)
    yt.run_map(mapper, src, dst, spec={'job_count': job_count})


if __name__ == '__main__':
    yt_main()
