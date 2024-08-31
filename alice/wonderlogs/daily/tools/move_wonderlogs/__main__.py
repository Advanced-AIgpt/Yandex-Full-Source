import datetime

import click

import yt.wrapper


@click.command()
@click.option('--wonderlogs', multiple=True)
@click.option('--new-wonderlogs', multiple=True)
@click.option('--backup-wonderlogs', multiple=True)
@click.option('--yt-token')
@click.option('--check-rows-count', is_flag=True)
@click.option('--check-tables-size', is_flag=True)
def main(wonderlogs, new_wonderlogs, backup_wonderlogs, yt_token, check_rows_count, check_tables_size):
    yt.wrapper.config['token'] = yt_token
    yt.wrapper.config.set_proxy('hahn')

    for old_w, new_w, backup_w in zip(wonderlogs, new_wonderlogs, backup_wonderlogs):
        print(old_w, new_w, backup_w)
        new_count = yt.wrapper.row_count(old_w)
        old_count = yt.wrapper.row_count(new_w)
        if check_rows_count:
            assert new_count == old_count, 'new_count: {}, old_count: {}'.format(new_count, old_count)
        old_size = yt.wrapper.get_attribute(old_w, 'data_weight')
        new_size = yt.wrapper.get_attribute(new_w, 'data_weight')
        if check_tables_size:
            assert abs(
                old_size - new_size) / old_size < 0.01, (
                'diff must be less than 1 percent old_size: {}, new_size: {}').format(
                old_size, new_size)

        yt.wrapper.move(old_w, backup_w, force=True, recursive=True)
        yt.wrapper.move(new_w, old_w, force=True, recursive=True)
        yt.wrapper.set(backup_w + '/@expiration_time',
                       (datetime.datetime.now() + datetime.timedelta(days=14)).isoformat())


if __name__ == '__main__':
    main()
