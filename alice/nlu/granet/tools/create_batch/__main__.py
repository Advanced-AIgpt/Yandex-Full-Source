# coding: utf-8

import click
import json
import os
import pathlib
import shutil
from alice.nlu.granet.tools.create_batch.utils import (
    load_lines,
    save_lines,
    create_yt_client,
    is_yt_table_exists,
    get_yt_table_column_names
)


def _escape_string(text):
    return json.dumps(str(text), ensure_ascii=False)[1:-1]


def _make_template_ctx(**kwargs):
    return {key: _escape_string(value) for key, value in kwargs.items()}


def _make_template_substitutions(dir, template_ctx):
    for path in pathlib.Path(dir).glob('**/*.template'):
        text = path.read_text()
        for key, value in template_ctx.items():
            text = text.replace('{' + key + '}', value)
        path.write_text(text)
        path.rename(path.with_name(path.stem))


def _copy_tree_with_replace(src_path, dst_path):
    if os.path.exists(dst_path):
        shutil.rmtree(dst_path)
    shutil.copytree(src_path, dst_path)


def _create_batch(batches_dir, batch_name, template_ctx):
    batch_path = os.path.join(batches_dir, batch_name)
    template_path = os.path.join(batches_dir, 'template')
    _copy_tree_with_replace(template_path, batch_path)

    _make_template_substitutions(batch_path, template_ctx)

    return batch_path


def _normalize_text(text):
    for c in '\'()':
        text = text.replace(c, ' ')
    text = ' '.join(text.split())
    return text.lower()


def _adjust_is_negative_column(name, yt_path, columns):
    if name is None and 'is_negative_query' in columns:
        name = 'is_negative_query'
    if name and name not in columns:
        raise ValueError('Column "%s" is not found in table "%s".' % (name, yt_path))
    return name


def _load_from_yt(yt_path, yt_proxy=None, yt_token=None, text_column=None, is_negative_column=None):
    yt_client = create_yt_client(yt_proxy, yt_token)

    if not is_yt_table_exists(yt_path, yt_client):
        raise OSError('YT table not found: "%s".' % yt_path)

    columns = get_yt_table_column_names(yt_path, yt_client)
    if text_column not in columns:
        raise ValueError('Column "%s" is not found in table "%s".' % (text_column, yt_path))
    is_negative_column = _adjust_is_negative_column(is_negative_column, yt_path, columns)

    positive = []
    negative = []
    ignored = []
    for row in yt_client.read_table(yt_path):
        text = _normalize_text(row.get(text_column, ''))
        if not text:
            continue
        if not is_negative_column:
            positive.append(text)
        elif row[is_negative_column] is None:
            # Queries from dialog history are stored with is_negative_query==null
            ignored.append(text)
        elif row[is_negative_column]:
            negative.append(text)
        else:
            positive.append(text)
    return positive, negative, ignored


def _write_batch_to_parent_ya_make(batches_dir, batch_name):
    ya_make_path = os.path.join(batches_dir, 'ya.make')
    lines = load_lines(ya_make_path)
    begin = lines.index('PEERDIR(') + 1
    end = lines.index(')', begin)
    peerdirs = set(lines[begin:end])
    peerdirs.add('    alice/nlu/data/ru/test/granet/tom/' + batch_name)
    lines = lines[:begin] + sorted(peerdirs) + lines[end:]
    save_lines(lines, ya_make_path)


@click.command()
@click.option(
    '--owner', required=True,
    help='User name to be written to OWNER in ya.make file.')
@click.option(
    '--batches-dir', type=click.Path(exists=True), required=True,
    help='Path to directory of custom batches (alice/nlu/data/ru/test/granet/custom)')
@click.option(
    '--batch-name', required=True,
    help='Name of created batch (name of subdirectory in directory of custom batches).')
@click.option(
    '--form-name', required=True,
    help='Name of form.')
@click.option(
    '--yt-path', required=True,
    help='Path to source dataset on YT.')
@click.option(
    '--yt-proxy', default='hahn', show_default=True,
    help='YT proxy.')
@click.option(
    '--yt-token',
    help='YT token.')
@click.option(
    '--text-column', default='text', show_default=True,
    help='Name of column with text.')
@click.option(
    '--is-negative-query-column',
    help='Name of column with labeles of negative samples. If the option is not defined the utility tries to use "is_negative_query" column.')
def create_tom(owner, batches_dir, batch_name, form_name, yt_path, yt_proxy, yt_token, text_column, is_negative_query_column):
    print('Create batch:')
    template_ctx = _make_template_ctx(owner=owner, form_name=form_name, src_path=yt_path)
    batch_path = _create_batch(batches_dir, batch_name, template_ctx)
    print('  Batch path: %s' % batch_path)

    print('Load source dataset:')
    print('  Path: %s' % yt_path)
    positive, negative, ignored = _load_from_yt(yt_path, yt_proxy, yt_token, text_column, is_negative_query_column)
    print('  Positive: %d. Negative: %d. Ignored: %d' % (len(positive), len(negative), len(ignored)))

    save_lines(['text'] + positive, os.path.join(batch_path, 'dataset/tom_pos.tsv'))
    save_lines(['text'] + negative, os.path.join(batch_path, 'dataset/tom_neg.tsv'))

    _write_batch_to_parent_ya_make(batches_dir, batch_name)


@click.group()
def main():
    pass


main.add_command(create_tom)

if __name__ == '__main__':
    main()
