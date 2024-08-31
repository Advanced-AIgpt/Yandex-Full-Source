import collections
import logging
import re
import requests


def find_containers_in_nanny_group(group_name):
    logging.info('Trying to get info about nanny group %s', group_name)
    url = 'https://nanny.yandex-team.ru/v2/services/{}/current_state/instances/'.format(group_name)
    response = requests.get(url, verify=False)
    response.raise_for_status()
    assert not response.content.startswith('<html>')
    return find_containers(group_name, response.json())


def find_containers(group_name, nanny_response):
    group_hosts = collections.defaultdict(list)
    data = [item for item in nanny_response['result'] if item]
    for entry in data:
        container = 'container_hostname' if 'MTN_ENABLED' in entry['network_settings'] else 'hostname'
        dc = [tag[-3:] for tag in entry['itags'] if 'a_dc_' in tag][0]
        if re.match(r'.+\.yp-c\.yandex\.net', entry[container]):  # <pod_id>.<dc>.yp-c.yandex.net
            try:
                dc = entry[container].split('.')[-4]
                logging.info('Detected YP host, extracted DC %s from container name %s', dc, entry[container])
            except Exception, ex:
                logging.warning('It seems we\'ve got YP host but name %s does not match expected format: %s', entry[container], ex)

            group_hosts[dc].append({'host': entry[container], 'port': entry['port']})
    logging.info('Hosts for group %s are %s', group_name, group_hosts)
    return group_hosts
