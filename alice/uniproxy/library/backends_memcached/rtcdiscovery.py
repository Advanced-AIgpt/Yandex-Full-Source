import os
import json

import tornado.httpclient

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


# ====================================================================================================================
class RtcDiscovery(object):
    def __init__(self, *args, **kwargs):
        super(RtcDiscovery, self).__init__()
        self._clusters = {}
        self._log = Logger.get('.rtcdisco')
        self._geo = self.__get_current_location(kwargs.get('default_location', 'sas'))
        self._crossdc = kwargs.get('cross_dc', True)
        self._locations = kwargs.get('locations', ['sas', 'man', 'vla'])

    # ----------------------------------------------------------------------------------------------------------------
    def __get_current_location(self, default_location=None):
        rtc_bsconfig_tags = os.environ.get('BSCONFIG_ITAGS', '').split()
        for tag in rtc_bsconfig_tags:
            if tag.startswith('a_geo_'):
                self._log.info('current location tag is "{}"'.format(tag))
                return tag[6:]
        self._log.debug('no location tag found, returning default ({})'.format(default_location))
        return default_location

    # ----------------------------------------------------------------------------------------------------------------
    async def __get_instances(self, client, cluster, service):
        request = tornado.httpclient.HTTPRequest(
            'https://hq.%s-swat.yandex-team.ru/rpc/instances/FindInstances/' % (cluster),
            method='POST',
            headers={
                'Content-Type': 'application/json',
            },
            body=json.dumps({
                'filter': {
                    'serviceId': service,
                }
            }),
            validate_cert=False
        )

        instances = []

        try:
            response = await client.fetch(request)

            self._log.info('fetch result {}'.format(response.code))

            data = json.loads(response.body.decode('utf-8'))

            data_instances = data.get('instance', [])

            for instance in data_instances:
                hostname = instance.get('spec', {}).get('hostname') + ':80'
                self._log.info('instance of service "{}" was found: {}'.format(service, hostname))
                instances.append(hostname)

        except tornado.httpclient.HTTPError as err:
            self._log.error(err)

        return instances

    # ----------------------------------------------------------------------------------------------------------------
    def _reorder_instances(self, hosts):
        dc_host_map = {}

        for dc, host in [(host[:3], host) for host in hosts]:
            if dc not in dc_host_map:
                dc_host_map[dc] = []
            dc_host_map[dc].append(host)

        reordered_hosts = []

        dcs = sorted(dc_host_map.keys())
        for i in range(0, len(hosts)):
            dc_index = i % len(dcs)
            host = dc_host_map[dcs[dc_index]].pop()
            reordered_hosts.append(host)

        return reordered_hosts

    # ----------------------------------------------------------------------------------------------------------------
    async def get_memcached_insances(self, service_id=None, memcached_id=None):
        if service_id is None:
            if memcached_id is None:
                service_id = config.get('memcached', {}).get('service_id', service_id)
            else:
                service_id = config.get('memcached', {}).get(memcached_id, {}).get('service_id', service_id)

        if service_id is None:
            return []

        client = tornado.httpclient.AsyncHTTPClient(force_instance=True)
        instances = []

        if isinstance(service_id, str):
            if self._crossdc:
                for location in self._locations:
                    instances_part = await self.__get_instances(client, location, service_id)
                    instances = instances + instances_part
            else:
                instances = await self.__get_instances(client, self._geo, service_id)

        elif isinstance(service_id, (list, tuple)):
            for service in service_id:
                if self._crossdc:
                    for location in self._locations:
                        instances_part = await self.__get_instances(client, location, service)
                        if instances_part:
                            instances += instances_part
                else:
                    instances_part = await self.__get_instances(client, self._geo, service)
                    if instances_part:
                        instances += instances_part

        if self._crossdc:
            instances = self._reorder_instances(instances)

        self._log.info('memcached instances -> {}'.format(instances))

        return instances


# ====================================================================================================================
if __name__ == '__main__':
    import argparse
    import logging
    import tornado.ioloop

    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s'
    )

    async def main():
        try:
            parser = argparse.ArgumentParser()
            parser.add_argument('-g', '--geo', help='geo location: man, vla, sas', required=True)
            parser.add_argument('-c', '--cross-dc', help='enable cross dc connections', action='store_true')
            parser.add_argument('-s', '--service', help='service: mssngr or alice', default='mssngr')
            args = parser.parse_args()

            discoveryClient = RtcDiscovery(
                default_location=args.geo,
                cross_dc=args.cross_dc,
                locations=['sas', 'man', 'vla']
            )

            instances = await discoveryClient.get_memcached_insances([
                '{}_memcached_{}'.format(args.service, geo) for geo in ('sas', 'man', 'vla')
            ])

            for instance in instances:
                print('found memcached instance: {}'.format(instance))
        except Exception as ex:
            print(ex)

    tornado.ioloop.IOLoop.current().run_sync(main)
