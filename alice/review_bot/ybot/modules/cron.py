# coding: utf-8

""" Sample config

ybot.modules.cron:
    tz: Europe/Moscow
    schedule:
    -
        conf: '15 12 * * 1-5'
        emit: my_module.event1
        value: обед

    -
        conf: '10 18 * * 1-5'
        emit: my_module.event2
        value: test

"""

from datetime import datetime

import attr
import gevent
import pytz
from crontab import CronTab

from ..core.events import emitter


MODULE_NAME = 'ybot.modules.cron'


@attr.s
class CronItem(object):
    cron = attr.ib()
    event = attr.ib()
    value = attr.ib()


@emitter(multi=True, check=False)
def cron_scheduler(ctx):
    conf = ctx.config.get(MODULE_NAME, [])
    tz = pytz.timezone(conf['tz'])

    schedule = []
    for item in conf['schedule']:
        cron_item = CronItem(
            cron=CronTab(item['conf']),
            event=item['emit'],
            value=item.get('value'),
        )
        schedule.append(cron_item)

    while schedule:
        now = datetime.now(tz=tz)
        for item in schedule:
            if item.cron.test(now):
                yield item.event, item.value

        next_event = min(schedule, key=lambda x: x.cron.next())
        gevent.sleep(next_event.cron.next())
