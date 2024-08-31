import asyncio

from async_lru import alru_cache
from traitlets import Instance, Integer, Unicode, default
from traitlets.config import SingletonConfigurable

from .api_client import DNSApiClient


class DNSManager(SingletonConfigurable):
    user_zone = Unicode(config=True)
    zone = Unicode('jupyter.yandex-team.ru', config=True)
    ttl = Integer(60, config=True)

    cache_ttl = Integer(10 * 60, config=True)

    dns_client = Instance(DNSApiClient)

    @default('dns_client')
    def _dns_client_default(self):
        return DNSApiClient.instance(parent=self)

    def make_primitive(self, left, right, operation):
        assert operation in ['add', 'delete']

        return {
            'name': left,
            'data': right,
            'type': 'CNAME',
            'operation': operation,
            'ttl': self.ttl,
        }

    def clear_records_cache(self):
        self.log.debug('clearing records cache')
        self.get_cached_records.cache_clear()

    @alru_cache(maxsize=None)
    async def get_cached_records(self):
        """
        После вызова метода, он будет возвращать кешированные значения
        self.cache_ttl секунд, затем очистится и повторный вызов приведет
        к созданию нового кеша, который опять очистится.

        """

        loop = asyncio.get_event_loop()
        loop.call_later(self.cache_ttl, self.clear_records_cache)

        return await self.get_records()

    def update_cname(self, user, set_callback):
        """
        Этот метод зовется из QYPSpawner.poll.
        Т.к. poll - метод синхронный и он ничего не знает о соседних спавнерах,
        мы тут делаем странные вещи:
        1) используем кеш записей, который обновляем раз в self.cache_ttl
        2) Синхронно создаем таск на проверку CNAME, который никогда не дожидаемся
          и надеемся, что он выполнится.

        """

        async def set_cname():
            dns_left = f'{user}.{self.user_zone}.'
            if dns_left in await self.get_cached_records():
                set_callback(dns_left)
            else:
                set_callback(None)

        asyncio.create_task(set_cname())

    async def get_records(self):
        """record выглядит следующим образом:
          {'left-side': 'lipkin.test.jupyter.yandex-team.ru.',
           'right-side': 'testing-jupyter-cloud-lipkin.vla.yp-c.yandex.net.',
           'ttl': '60',
           'type': 'CNAME'},

        Т.е. left-side - это CNAME, который мы назначаем, а right-side -
        назначение этого CNAME.


        """
        zone_id = await self.dns_client.get_zone_id(self.zone)
        raw_records = await self.dns_client.get_zone_records(zone_id)
        records = [
            record
            for record in raw_records
            if record['left-side'].endswith(f'{self.user_zone}.')
        ]

        return {
            record['left-side']: record
            for record in records
        }

    def generate_user_primitives(self, records, dns_left, dns_right):
        primitives = []
        if dns_left in records:
            record = records[dns_left]
            if record['right-side'] == dns_right:
                self.log.debug('no need to set CNAME %s = %s', dns_left, dns_right)
                return []

            self.log.debug('going to delete CNAME %s = %s', dns_left, record['right-side'])
            primitives.append(
                self.make_primitive(dns_left, record['right-side'], 'delete'),
            )

        self.log.debug('going to add CNAME %s = %s', dns_left, dns_right)
        primitives.append(
            self.make_primitive(dns_left, dns_right, 'add'),
        )

        return primitives

    async def set_user_dns(self, user, vm_host):
        dns_left = f'{user}.{self.user_zone}.'
        dns_right = f'{vm_host}.'

        records = await self.get_records()

        primitives = self.generate_user_primitives(records, dns_left, dns_right)
        if primitives:
            self.log.debug('applying next dns primitives: %s', primitives)
            await self.dns_client.apply_primitives(primitives)

        return dns_left

    async def sync_dns(self, users_hosts):
        records = await self.get_records()

        hosts_to_modify = set()
        hosts_unchanged = set()
        hosts_to_delete = set()

        primitives = []

        for user, vm_host in users_hosts.items():
            dns_left = f'{user}.{self.user_zone}.'
            dns_right = f'{vm_host}.'

            user_primitives = self.generate_user_primitives(records, dns_left, dns_right)
            if user_primitives:
                hosts_to_modify.add(dns_left)
                primitives.extend(user_primitives)
            else:
                hosts_unchanged.add(dns_left)

        for dns_left, record in records.items():
            if dns_left in hosts_to_modify or dns_left in hosts_unchanged:
                continue

            hosts_to_delete.add(dns_left)
            self.log.debug('going to delete CNAME %s = %s', dns_left, record['right-side'])

            delete = self.make_primitive(dns_left, record['right-side'], 'delete')
            primitives.append(delete)

        self.log.info(
            '%d unchanged hosts, %d to change (add or rename), %d to delete',
            len(hosts_unchanged), len(hosts_to_modify), len(hosts_to_delete),
        )

        if primitives:
            self.log.debug('applying next dns primitives: %s', primitives)
            await self.dns_client.apply_primitives(primitives)
        else:
            self.log.debug('dns already in sync with hosts')
