#!/usr/bin/env python
from nile.api.v1 import (
    clusters,
    filters as nf,
)
from utils.nirvana.op_caller import call_as_operation
import yt.wrapper as yt

def main(construct, model, size="10000", base_annotation_folder="//home/voice/toloka/ru-RU/daily/", output_path="//home/voice/nikitachizhov/wer_stable/"):

    if(construct != 'true'):
        return

    cluster = clusters.Hahn()
    yt.config.set_proxy("hahn")

    job = cluster.job()

    folder = base_annotation_folder.rstrip('/') + '/' + model.strip('/')
    annotations = job.table(folder + '/{' + ','.join(yt.list(folder)[-30:]) + '}')
    annotations = annotations.filter(nf.equals('mark', 'TEST'))
    annotations = annotations.top(int(size), by='ts')
    annotations = annotations.project('url', 'text', 'voices', 'mark')

    output = output_path.rstrip('/') + '/' + model.strip('/')
    annotations.put(output)

    job.run()

    return {"table":output}

if __name__ == "__main__":
    call_as_operation(main)