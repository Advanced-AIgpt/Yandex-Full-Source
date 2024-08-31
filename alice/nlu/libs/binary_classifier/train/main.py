# coding: utf-8

import click
import getpass
import logging
import uuid
import vh
import yt.wrapper as yt

from prepare_nlu_data import prepare_nlu_data
from train import TrainConfig
from vh_collect_embeddings_subgraph import build_boltalka_embeddings_collection_subgraph
from vh_train_subgraph import build_train_subgraph

logger = logging.getLogger(__name__)


def _prepare_data(config):
    if config.dataset.source == 'nlu':
        if config.dataset.yt_path is None:
            data_table = '//tmp/{}/{}'.format(getpass.getuser(), uuid.uuid4())
        else:
            data_table = config.dataset.yt_path
        if not yt.exists(data_table):
            prepare_nlu_data(data_table)
    elif config.dataset.source == 'yt':
        assert config.dataset.yt_path is not None
        data_table = config.dataset.yt_path
    else:
        assert False, 'Unknown data source {}'.format(config.dataset.source)

    return data_table


def _build_train_graph(config_path):
    config = TrainConfig.from_json(config_path)

    data_table = _prepare_data(config)

    row = next(yt.read_table(data_table))

    data_table = vh.YTTable(data_table)
    if config.model.embedding not in row:
        data_table = build_boltalka_embeddings_collection_subgraph(data_table, config.model.embedding)

    build_train_subgraph(config_path, data_table)


@click.command()
@click.option('--config-path', type=click.Path(exists=True), required=True)
@click.option('--yt-token', default=getpass.getuser() + '_yt_token', show_default=True)
@click.option('--yql-token', default=getpass.getuser() + '_yql_token', show_default=True)
@click.option('--mr-account', default='tmp', show_default=True)
@click.option('--yt-cluster', default='hahn', show_default=True)
@click.option('--yt-pool', default='default', show_default=True)
@click.option('--nirvana-quota', default='default', show_default=True)
@click.option('--sandbox-owner', default=getpass.getuser(), show_default=True)
@click.option('--sandbox-token', default=getpass.getuser() + '_sandbox_token', show_default=True)
def main(config_path, yt_token, yql_token, mr_account, yt_cluster,
         yt_pool, nirvana_quota, sandbox_owner, sandbox_token):
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    global_options = {
        'yt_token': yt_token,
        'yql_token': yql_token,
        'mr_account': mr_account,
        'yt_cluster': yt_cluster,
        'yt_pool': yt_pool,
        'sandbox_owner': sandbox_owner,
        'sandbox_token': sandbox_token,
    }

    _build_train_graph(config_path)

    vh.run(
        quota=nirvana_quota,
        yt_token_secret=global_options['yt_token'],
        yt_proxy=global_options['yt_cluster'],
        yt_pool=global_options['yt_pool'],
        mr_account=global_options['mr_account'],
        global_options=global_options,
        api_retries=100,
    )


if __name__ == "__main__":
    main()
