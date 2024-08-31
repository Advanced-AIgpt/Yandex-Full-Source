import argparse
import vh
import yt.wrapper as yt
import subprocess
import json
import yaml

yt.config.set_proxy('hahn.yt.yandex.net')


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=10000))
@vh.lazy(binary=vh.Executable, input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), prefix=vh.mkoption(str), shards_number=vh.mkoption(str, default='1'))
def make_items(binary, input_table, prefix, shards_number, output_table):
    command = [binary, '-i', str(input_table), '-o', str(output_table), '-s', shards_number, '-v', '100000000']
    if prefix:
        prefix = prefix.strip(':')
        command += ['--column-prefix', prefix]
    return subprocess.check_call(command)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=10000))
@vh.lazy(binary=vh.Executable, input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), extra_params=vh.mkoption(str, default=''))
def build_hnsw(binary, input_table, extra_params, output_table):
    return subprocess.check_call([binary, 'build', '-p', 'hahn', '-v', str(input_table), '-o', str(output_table), '-t', 'float', '-d',
                                  '600', '-D', 'dot_product', '--cpu-limit',  '1', '-b', '30000', '-c', '1000', '-s', '400'] + extra_params.split())


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=10000))
@vh.lazy(binary=vh.Executable, input_table=vh.YTTable, output_path=vh.mkoutput(vh.File), offsets_path=vh.mkoutput(vh.File))
def download_all_shards_hnsw(binary, input_table, output_path, offsets_path):
    return subprocess.check_call([binary, 'download-all-shards', '-p', 'hahn', '-i', str(input_table), '-o', output_path, '-f', offsets_path])


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=10000))
@vh.lazy(binary=vh.Executable, input_table=vh.YTTable, output_file=vh.mkoutput(vh.File))
def download_hnsw(binary, input_table, output_file):
    return subprocess.check_call([binary, 'download-shard', '-p', 'hahn', '-i', str(input_table), '-o', str(output_file), '-s', '0'])


def get_hnsw_path(index_name, model_name, file_type):
    file_type = 'hnsw' if file_type == 'es' else 'offsets'
    index_name = index_name.replace('/', '_')
    model_name = model_name.rsplit('/', 1)[-1]
    return f'//home/dialogs/boltalka/models/hnsw/{index_name}_{model_name}.{file_type}'


def upload_hnsw_data(file_name, index, model, file_type='es'):
    hnsw_path = get_hnsw_path(index['table'], model['id'], file_type)
    yt.write_file(hnsw_path, open(file_name, 'rb'))
    hnsw_data = yt.get_attribute(index['table'], '_hnsw_index{}'.format(file_type))
    hnsw_data[model['id']] = hnsw_path
    yt.set_attribute(index['table'], '_hnsw_index{}'.format(file_type), hnsw_data)


def upload_hnsw_data_nv(f, index, model, file_type='es'):
    hnsw_path = get_hnsw_path(index['table'], model['id'], file_type)
    mr_upload_file_op = vh.op(id="fe5be04f-5e6b-494a-8673-961535b036a3").partial(yt_token=vh.get_yt_token_secret())
    op = mr_upload_file_op(content=f, mr_default_cluster='hahn', dst_path=hnsw_path, write_mode='OVERWRITE')
    return op, {model['id']: hnsw_path}


def build(args):
    config = yaml.load(open(args.config))
    make_items_bin = vh.arcadia_executable('alice/boltalka/tools/make_items_table_from_shards', revision=7671799)
    build_dense_bin = vh.arcadia_executable('cv/dialogs/nlg/hnsw/tools/yt_build_dense_vector_index2', revision=3713463)
    mr_set_attr_op = vh.op(id="5a2ce930-a24f-454c-82ed-f1cab709165e").partial(yt_token=vh.get_yt_token_secret(), mr_account='tmp')
    indexes = [(index, False) for index in config['indexes']] + [(index, True) for index in config['entity_indexes']]
    for index, is_entity_index in indexes:
        if not yt.has_attribute(index['table'], '_hnsw_indexes'):
            yt.set_attribute(index['table'], '_hnsw_indexes', {})
        if is_entity_index and not yt.has_attribute(index['table'], '_hnsw_index_offsets'):
            yt.set_attribute(index['table'], '_hnsw_index_offsets', {})
        hnsw_indexes = yt.get_attribute(index['table'], '_hnsw_indexes')
        if is_entity_index:
            hnsw_index_offsets = yt.get_attribute(index['table'], '_hnsw_index_offsets')
        uploaders = []
        for model in config['models']:
            if not model['id'] in hnsw_indexes:
                table_size = yt.row_count(index['table'])
                last_row = next(yt.read_table(yt.TablePath(index['table'], exact_index=table_size-1), format=yt.YsonFormat(encoding=None)))
                shards_number = last_row[b'shard_id'] + 1
                assert shards_number == 1 or is_entity_index
                items = make_items(make_items_bin, vh.YTTable(index['table']), model['prefix'], str(shards_number))
                # TODO: get rid of condition
                if index['name'] == 'proactivity':
                    items = build_hnsw(build_dense_bin, items, extra_params='-m 1')
                elif is_entity_index:
                    items = build_hnsw(build_dense_bin, items, extra_params='-m 16')
                else:
                    items = build_hnsw(build_dense_bin, items)
                if is_entity_index:
                    items = download_all_shards_hnsw(build_dense_bin, items)
                    index_data, offsets_data = items[0], items[1]
                else:
                    items = download_hnsw(build_dense_bin, items)
                    index_data, offsets_data = items, None
                op, uploaded = upload_hnsw_data_nv(index_data, index, model)
                uploaders.append(op)
                hnsw_indexes.update(uploaded)
                if offsets_data is not None:
                    op, uploaded = upload_hnsw_data_nv(offsets_data, index, model, '_offsets')
                    uploaders.append(op)
                    hnsw_index_offsets.update(uploaded)
                print('HNSW built for index {}, model {}'.format(index['name'], model['name']))
            else:
                print('Found existing index {}, model {}'.format(index['name'], model['name']))
        yt_table = vh.YTTable(index['table'])
        mr_set_attr_op(_after=uploaders, path=yt_table, attribute='_hnsw_indexes', value=vh.data_from_str(json.dumps(hnsw_indexes)))
        if is_entity_index:
            mr_set_attr_op(_after=uploaders, path=yt_table, attribute='_hnsw_index_offsets', value=vh.data_from_str(json.dumps(hnsw_index_offsets)))
    vh.run(wait=args.wait, quota=args.quota, api_retries=-1)


def upload(args):
    config = yaml.load(open(args.config))
    index = [index for index in config['indexes'] if index['name'] == args.index][0]
    if not yt.has_attribute(index['table'], '_hnsw_indexes'):
        yt.set_attribute(index['table'], '_hnsw_indexes', {})
    model = [model for model in config['models'] if model['name'] == args.model][0]
    upload_hnsw_data(args.file, index, model, args.quota)


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('mode')
    parser.add_argument('--config')
    parser.add_argument('--file')
    parser.add_argument('--index')
    parser.add_argument('--model')
    parser.add_argument('--quota', default='dialogs')
    parser.add_argument('--patch')
    parser.add_argument('--wait', action='store_true')
    args = parser.parse_args()
    if args.mode == 'build':
        build(args)
    elif args.mode == 'upload':
        upload(args)
    else:
        print("Unknown mode")
