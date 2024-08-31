import getpass
import uuid

import click
import yt.wrapper.config

from alice.beggins.cmd.manifestator.lib.process import process_manifest


def get_tmp_table(prefix):
    return f'//tmp/{getpass.getuser()}/{prefix}_{uuid.uuid4().hex}'


@click.command()
@click.option('--manifest', required=True)
@click.option('--proxy', default='hahn')
def main(manifest, proxy):
    yt.wrapper.config['proxy']['url'] = proxy
    train = get_tmp_table(prefix='train')
    accept = get_tmp_table(prefix='accept')
    process_manifest(
        manifest,
        train_table=train,
        accept_table=accept,
    )
    print(f'Tables: \n{train}\n{accept}')


if __name__ == '__main__':
    main()
