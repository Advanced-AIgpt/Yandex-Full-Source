import time

import click
from alice.analytics.operations.dialog.sessions.lib_for_py2.make_dialog_sessions import main as run


@click.command()
@click.option('--date', required=True)
@click.option('--sessions-root', required=True)
@click.option('--expboxes', required=True)
@click.option('--users-path', required=True)
@click.option('--devices-path', required=True)
@click.option('--yql-token', required=True)
@click.option('--pool', required=True)
def main(date, sessions_root, expboxes, users_path, devices_path, yql_token, pool):
    st = time.time()
    print('start at', time.ctime(st))

    custom_cluster_params = dict(yql_token=yql_token) if yql_token else None
    run(
        date=date,
        sessions_root=sessions_root,
        expboxes=expboxes,
        users_path=users_path,
        devices_path=devices_path,
        custom_cluster_params=custom_cluster_params,
        pool=pool,
        include_utils=False,
        neighbour_names=[]
    )

    print('total elapsed {:2f} min'.format((time.time() - st) / 60))


if __name__ == '__main__':
    main()
