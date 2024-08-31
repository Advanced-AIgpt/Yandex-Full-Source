import argparse
import json
import logging

import os
import requests
import yt.wrapper as yt
from nirvana_api import NirvanaApi
from nirvana_api.blocks import BaseBlock, ProcessorType
from nirvana_api.highlevel_api import create_workflow, run_workflow
from requests.packages.urllib3.exceptions import InsecureRequestWarning

logger = logging.getLogger(__name__)
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.INFO)

COLUMNS = [
    ('request_id', 'string'),
    ('text', 'string'),
    ('app', 'any'),
    ('device_state', 'any'),
    ('features', 'any'),
    ('skip', 'boolean')
]

DEFAULT_VALUES = [
    ('skip', False)
]

OPERATION_NAME = 'VINS Marker Tests'

TASK_STATUS_TEMPLATE = ('<html>'
                        '<head>'
                        '<meta http-equiv="refresh" content="0; url="{url}"/>'
                        '</head>'
                        '<body>'
                        '<script>location="{url}"</script>'
                        '</body>'
                        '</html>')


def get_schema():
    schema = []
    for name, type_ in COLUMNS:
        res = {'name': name, 'type': type_}
        schema.append(res)

    return schema


def fill_default(item):
    for name, val in DEFAULT_VALUES:
        if name not in item:
            item[name] = val
    return item


def load_basket(basket):
    data = json.load(basket)
    appropriate_keys = [name for name, _ in COLUMNS]
    for row in data:
        yield fill_default({k: v for k, v in row.iteritems() if k in appropriate_keys})


def get_input():
    parser = argparse.ArgumentParser()
    parser.add_argument('--basket', required=True, help='File path to requests basket')
    parser.add_argument('--cluster', default='hahn', help='Cluster')
    parser.add_argument('--vins_url', required=True, help='VINS Url')
    parser.add_argument('--yt_token', help='YT Token', default=None)
    parser.add_argument('--nirvana_token', help='NIRVANA Token', default=None)
    parser.add_argument('--gen_artifact', help='Generate Nirvana Artifact in TeamCity',
                        required=False, default=False, action='store_true')
    args = parser.parse_args()

    cluster = args.cluster
    yt_token = args.yt_token or os.environ['YT_TOKEN']
    nirvana_token = args.nirvana_token or os.environ['NIRVANA_TOKEN']

    yt.config['proxy']['url'] = cluster
    yt.config['token'] = yt_token

    with yt.Transaction():
        output_table = yt.create_temp_table(attributes={'optimize_for': 'scan', 'schema': get_schema()},
                                            expiration_timeout=1000 * 60 * 60 * 24 * 2)
        with open(args.basket, 'r') as f:
            yt.write_table(output_table, load_basket(f))
        logger.info('Basket uploaded to {0}.[{1}]'.format(cluster, output_table))

    return NirvanaApi(nirvana_token, ssl_verify=False), cluster, output_table, args.vins_url, args.gen_artifact


class MarkerTests(BaseBlock):
    processor_type = ProcessorType.Job
    output_names = ['output']
    parameters = ['vins_url', 'input_table', 'cluster']


def make_artifact(url, file_name='nirvana_status.html'):
    print 'NIRVANA WORKFLOW URL: {0}'.format(url)
    with open(file_name, 'w') as f:
        f.write(TASK_STATUS_TEMPLATE.format(url=url))


def main():
    requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
    api, cluster, table, vins_url, need_artifact = get_input()
    operation_id = api.find_operation(OPERATION_NAME)[0].operationId
    graph = MarkerTests(
        guid=operation_id,
        name=OPERATION_NAME,
        vins_url=vins_url,
        input_table=table,
        cluster=cluster
    )
    workflow = create_workflow(api, 'VINS Marker Tests', graph)
    workflow_url = workflow.get_url()
    logger.info('Created workflow: %s' % workflow_url)
    if need_artifact:
        make_artifact(workflow_url)
    run_workflow(workflow)


if __name__ == '__main__':
    main()
