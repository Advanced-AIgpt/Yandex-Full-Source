import argparse
import logging
from os.path import join as pj

from alice.protos.api.conjugator.api_pb2 import TConjugateRequest, TConjugateResponse
from alice.protos.data.language.language_pb2 import ELang
from apphost.python.client.client import Client as ApphostClient
from yt.wrapper import YtClient


YT_CLUSTER = 'hahn'
LANGUAGE = ELang.L_ARA
CONJUGATOR_URL = 'ar-sklonyator-1.sas.yp-c.yandex.net:81/conjugate'


def prepare_output_table(yt_client, input_table, output_table):
    yt_client.remove(output_table, force=True)
    schema = yt_client.get(pj(input_table, '@schema'))
    schema.append({'name': 'conjugated_phrase', 'type': 'string'})
    yt_client.create('table', output_table, recursive=True, attributes={"schema": schema})


def split_conjugator_url(conjugator_url):
    path = '/conjugate'
    host_port_path = conjugator_url.split('/', maxsplit=1)
    if len(host_port_path) == 2:
        path = '/' + host_port_path[1]

    port = 81
    host_port = host_port_path[0].split(':')
    if len(host_port) == 2:
        port = int(host_port[1])

    host = host_port[0]
    return (host, port, path)


def conjugate(rows, conjugator_url):
    conjugator_request = TConjugateRequest(Language=LANGUAGE)
    for row in rows:
        conjugator_request.UnconjugatedPhrases.append(row['unconjugated_phrase'])

    host, port, path = split_conjugator_url(conjugator_url)
    conjugator_response = ApphostClient(host, port).request(
        path=path, conjugator_request=conjugator_request
    ).get_only_type_item('conjugator_response', TConjugateResponse)
    assert len(rows) == len(conjugator_response.ConjugatedPhrases)

    for i in range(len(rows)):
        rows[i]['conjugated_phrase'] = conjugator_response.ConjugatedPhrases[i]


def do_work(input_table, output_table, conjugator_url):
    output_table = output_table or (input_table + '_conjugated')
    yt_client = YtClient(proxy=YT_CLUSTER)
    rows = list(yt_client.read_table(input_table))
    if not rows:
        logging.info('Nothing to conjugate')
        return

    conjugate(rows, conjugator_url)

    prepare_output_table(yt_client, input_table, output_table)
    yt_client.write_table(output_table, rows)
    logging.info('Successfully conjugated {} phrases'.format(len(rows)))


def main():
    logging.basicConfig(level=logging.INFO, format='[%(levelname)s]   %(message)s')

    parser = argparse.ArgumentParser(description='Conjugate phrases')
    parser.add_argument('-i', '--input', help='Path to input YT table with "unconjugated_phrase" column', required=True)
    parser.add_argument('-o', '--output', help='Path to output YT table with "conjugated_phrase" column', required=False)
    parser.add_argument('-u', '--conjugator-url', help='Url for conjugator backend', required=False, default=CONJUGATOR_URL)
    args = parser.parse_args()
    do_work(args.input, args.output, args.conjugator_url)


if __name__ == '__main__':
    main()
