import yaml
import sys
import os
import yt.wrapper as yt
import requests
import shutil
from itertools import count
from contextlib import ExitStack
import numpy as np
import subprocess
from urllib.parse import urlsplit


yt.config.set_proxy('hahn')
yt.config["read_parallel"]["enable"] = True
yt.config["read_parallel"]["max_thread_count"] = 10


def download_yt(path, filename):
    f = yt.read_file(path)
    with open(filename, 'wb') as of:
        shutil.copyfileobj(f, of)


def download_nirvana_data(id, filename):
    data = requests.get('https://nirvana.yandex-team.ru/api/storedData/{}/data'.format(id), stream=True, verify=False)
    with open(filename, 'wb') as f:
        shutil.copyfileobj(data.raw, f)


def sky_get(sbr, base_dir):
    subprocess.check_call('sky get -d {} sbr:{}'.format(base_dir, sbr), shell=True)


def parse_columns(row, prefix):
    columns = []
    for i in count():
        k = '{}_{}'.format(prefix, i).encode()
        if k not in row:
            break
        columns.append(row[k])
    return columns


def get_required_embeddings(index_type):
    if index_type == 'context_and_reply':
        return [b'context_embedding', b'reply_embedding']
    else:
        return [bytes(index_type + '_embedding', encoding='utf-8')]


class GifIndex:
    def __init__(self, gif_tables):
        self._gif_index = {}
        for gif_table in gif_tables:
            for row in yt.read_table(gif_table['table']):
                reply = row['rewritten_reply'].encode('utf-8')
                self._gif_index[reply] = self._parse_gifs(row['gifs'])

    @classmethod
    def _parse_gifs(cls, gifs):
        gif_source_urls = []
        gif_source_texts = []
        gif_urls = []
        for gif in gifs:
            gif_urls.append(gif[0][0])
            gif_source_urls.append(gif[1][0])
            gif_source_texts.append(urlsplit(gif[2][0]).netloc)
        return (
            ' '.join(gif_source_urls).encode('utf-8'),
            ' '.join(gif_source_texts).encode('utf-8'),
            ' '.join(gif_urls).encode('utf-8')
        )

    def get_gifs(self, reply):
        return self._gif_index.get(reply, (b'', b'', b''))


class FileList:
    def __init__(self, files):
        self.files = files

    def __iter__(self):
        return iter(self.files)

    def __exit__(self):
        for el in self.files:
            el.close()


def main():
    config, output_dir, indexer = sys.argv[1:]
    config = yaml.load(open(config))
    os.mkdir(output_dir)
    for resource in config['extra_resources']:
        sky_get(resource, output_dir)
    for ranker in config['rankers']:
        download_yt(ranker['id'], os.path.join(output_dir, ranker['name'] + '.info'))
        pass
    download_yt(config['ruslister'], os.path.join(output_dir, 'ruslister_map.txt'))
    models = config['models']
    indexes = config['indexes']
    entity_indexes = config.get('entity_indexes', [])
    for model in models:
        os.mkdir(os.path.join(output_dir, model['name']))
        download_yt(model['id'], os.path.join(output_dir, model['name'], 'model'))
        for index in indexes + entity_indexes:
            index_type = index.get('index_type', 'context_and_reply')
            os.mkdir(os.path.join(output_dir, model['name'], index['name']))
            if not yt.has_attribute(index['table'], '_hnsw_indexes'):
                yt.set_attribute(index['table'], '_hnsw_indexes', {})
            hnsw_indexes = yt.get_attribute(index['table'], '_hnsw_indexes')
            if not model['id'] in hnsw_indexes:
                print('HNSW index meta missing')
                sys.exit(57)
            download_yt(hnsw_indexes[model['id']], os.path.join(output_dir, model['name'], index['name'], '{}.index'.format(index_type)))
        for index in entity_indexes:
            index_type = index.get('index_type', 'context_and_reply')
            if not yt.has_attribute(index['table'], '_hnsw_index_offsets'):
                yt.set_attribute(index['table'], '_hnsw_index_offsets', {})
            hnsw_index_offsets = yt.get_attribute(index['table'], '_hnsw_index_offsets')
            if not model['id'] in hnsw_index_offsets:
                print('HNSW index offset meta missing')
                sys.exit(57)
            download_yt(hnsw_index_offsets[model['id']], os.path.join(output_dir, model['name'], index['name'], '{}.index_offsets'.format(index_type)))
    with ExitStack() as stack:
        static_factors = stack.enter_context(open(os.path.join(output_dir, 'static_factors.bin'), 'wb'))
        texts = stack.enter_context(open(os.path.join(output_dir, 'context_and_reply.txt'), 'wb'))
        doc_id_offset = 0
        max_doc_id = -1

        prefixes = [model['prefix'].encode("utf-8") for model in models]
        index_list = [(index, False) for index in indexes] + [(index, True) for index in entity_indexes]
        gif_index = GifIndex(config.get('gifs', []))
        for index, is_entity_index in index_list:
            index_type = index.get('index_type', 'context_and_reply')
            required_embeddings = get_required_embeddings(index_type)
            doc_id_offset = max_doc_id + 1
            index_offset = doc_id_offset
            dirs = [os.path.join(output_dir, model['name'], index['name']) for model in models]
            vectors_files = [stack.enter_context(open(os.path.join(d, '{}.vec'.format(index_type)), 'wb')) for d in dirs]
            vector_offset_files = []
            ids_files = [stack.enter_context(open(os.path.join(d, '{}.ids'.format(index_type)), 'wb')) for d in dirs]
            ids_offset_files = []
            entities_files = []
            embedding_sizes = {}
            if is_entity_index:
                entities_files = [stack.enter_context(open(os.path.join(d, 'entities'), 'wb')) for d in dirs]
                vector_offset_files = [stack.enter_context(open(os.path.join(d, '{}.vec_offsets'.format(index_type)), 'wb')) for d in dirs]
                ids_offset_files = [stack.enter_context(open(os.path.join(d, '{}.ids_offsets'.format(index_type)), 'wb')) for d in dirs]
            current_entity = None
            for row in yt.read_table(index['table'], format=yt.YsonFormat(encoding=None)):
                if not embedding_sizes:
                    for prefix in prefixes:
                        embedding_sizes[prefix] = len(row[prefix + b'context_embedding'])
                static_factors.write(np.array(parse_columns(row, 'static_factor'), dtype=np.float32).tobytes())
                texts.write(b'\t'.join(list(reversed(parse_columns(row, 'context'))) + [row[b'rewritten_reply'],
                                                                                        row.get(b'disrespect_reply', b''),
                                                                                        row[b'source'],
                                                                                        row.get(b'proactivity_action', b''),
                                                                                        *gif_index.get_gifs(row[b'rewritten_reply'])]))
                texts.write(b'\n')
                if is_entity_index and current_entity != row[b'entity_id']:
                    for enitities_file in entities_files:
                        enitities_file.write(row[b'entity_id'])
                        enitities_file.write(b'\n')
                    for ids_offset_file in ids_offset_files:
                        ids_offset_file.write(np.array([(max_doc_id + 1 - index_offset) * 4], dtype=np.uint64).tobytes())
                    for vector_offset_file, model_name in zip(vector_offset_files, prefixes):
                        embedding_size = embedding_sizes[model_name]
                        if index_type == "context_and_reply":
                            embedding_size *= 2
                        vector_offset_file.write(np.array([(max_doc_id + 1 - index_offset) * embedding_size], dtype=np.uint64).tobytes())
                    doc_id_offset = max_doc_id + 1
                    current_entity = row[b'entity_id']
                for prefix, ids, vectors in zip(prefixes, ids_files, vectors_files):
                    for embedding in required_embeddings:
                        vectors.write(row[prefix + embedding])
                    doc_id = doc_id_offset + row[b'doc_id']
                    ids.write(np.array([doc_id], dtype=np.uint32).tobytes())
                    max_doc_id = doc_id

    subprocess.check_call('{} -d {} < {}'.format(indexer, output_dir, os.path.join(output_dir, 'context_and_reply.txt')), shell=True)
