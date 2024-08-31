# -*- coding: utf-8 -*-

import os
from datetime import timedelta

from nile.api.v1 import (
    cli
)

from alice.analytics.operations.retention.stations import get_stations_retention_cube
from alice.analytics.operations.retention.tvandroid import get_tvandroid_retention_cube


def set_symlinks(driver, options):
    date = options.dates[0]

    dir_list = [
        '//home/sda/retention/cubes/stations',
        '//home/sda/retention/cubes/tvandroid'
    ]

    for dir_path in dir_list:
        dir_list = driver.list(dir_path)

        if 'last' in dir_list:
            dir_list.remove('last')

        last_date = max(dir_list)
        target_path = os.path.join(dir_path, options.dates[0])
        link_path = os.path.join(dir_path, 'last')

        if date >= last_date:
            driver.client.yt_client.link(target_path, link_path, recursive=False, ignore_existing=False, force=True)


@cli.statinfra_job(final_action=set_symlinks)
def make_job(job, nirvana, options):
    job_date = options.dates[0]

    stations_retention_cube = get_stations_retention_cube(job, job_date) \
        .sort('activation_date', 'device_type', 'user_id', 'fielddate') \
        .put(
            '//home/sda/retention/cubes/stations/{}'.format(job_date))

    tvandroid_retention_cube = get_tvandroid_retention_cube(job, job_date) \
        .sort('activation_date', 'device_type', 'user_id', 'fielddate') \
        .put(
            '//home/sda/retention/cubes/tvandroid/{}'.format(job_date))

    return job



if __name__ == '__main__':
    cli.run()
