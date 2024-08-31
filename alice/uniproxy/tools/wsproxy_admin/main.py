import json
import asyncio
import aiohttp
from .argparser import WsproxyArgparser


NANNY_INSTANCES_URL = 'https://nanny.yandex-team.ru/v2/services/{service}/current_state/instances/'
DISCONNECT_CGI_PARAMETER = '&filter_type={filter_type}&filter_value={filter_value}'
NANNY_REQUEST_RETRY_COUNT = 10
HOST_REQUEST_RETRY_COUNT = 10
INITIAL_NANNY_TIMEOUT = 0.05
INITIAL_HOST_TIMEOUT = 0.05
RETRY_COEFFICIENT = 1.5


class RequestArguments:
    _valid_services = {'wsproxy-vla', 'wsproxy-man', 'wsproxy-sas',
                       'wsproxy-prestable-sas', 'uniproxy2_man', 'uniproxy2_sas', 'uniproxy2_vla'}

    def __init__(self, args):
        self._action = args.action
        self._service = args.service.strip().split(',') if args.service else []
        for x in self._service:
            if x not in self._valid_services:
                raise ValueError(f'invalid service name {x}')
        if not args.host and not self._service:
            raise ValueError('hostnames or services must be specified')
        if args.host and self._service:
            raise ValueError('only one of service and host can be specified')
        self._fill_cgi_parameters(args)

    def _fill_cgi_parameters(self, args):
        if self.action == 'disconnect_clients':
            self._uuid = args.uuid
            self._surface = args.surface
            self._device_id = args.device_id
            self._session_id = args.session_id
            self._ratio = args.ratio
            if args.ratio and (args.ratio > 1.0 or args.ratio < 0):
                raise ValueError(
                    'invalid ratio value, it must be in range [0, 1]')
            params = list(
                filter(bool, [args.uuid, args.session_id, args.device_id, args.surface]))
            if len(params) > 1:
                raise ValueError(
                    'disconnect_clients can not have more than 1 filter')
            elif len(params) == 1:
                if args.surface and args.ratio is None:
                    raise ValueError('--ratio is required for --surface')
                elif not args.surface and args.ratio:
                    raise ValueError('--ratio is not permitted for any parameter besides --surface')
                self._action = 'disconnect_clients_with_filter'
            elif args.ratio is None:
                raise ValueError('--ratio is required for disconnect without parameters')

    @property
    def action(self):
        return self._action

    @property
    def ratio(self):
        return self._ratio

    @property
    def services(self):
        return self._service

    @property
    def uuid(self):
        return self._uuid

    @property
    def surface(self):
        return self._surface

    @property
    def device_id(self):
        return self._device_id

    @property
    def session_id(self):
        return self._session_id


def _update_url_to_admin(url):
    return f'http://{url}/admin'


def _build_url(url, request_args):
    result = f'{url}?action={request_args.action}'
    if request_args.action.startswith('disconnect_clients'):
        if request_args.ratio:
            result += f'&ratio={request_args.ratio}'
        if request_args.uuid:
            result += DISCONNECT_CGI_PARAMETER.format(
                filter_type='Uuid', filter_value=request_args.uuid)
        if request_args.surface:
            result += DISCONNECT_CGI_PARAMETER.format(
                filter_type='Surface', filter_value=request_args.surface)
        if request_args.device_id:
            result += DISCONNECT_CGI_PARAMETER.format(
                filter_type='DeviceId', filter_value=request_args.device_id)
        if request_args.session_id:
            result += DISCONNECT_CGI_PARAMETER.format(
                filter_type='SessionId', filter_value=request_args.session_id)
    return result


def _get_host_urls(text):
    return [_update_url_to_admin(instance["container_hostname"]) for instance in json.loads(text)['result']]


async def _get_instances(service):
    async with aiohttp.ClientSession() as session:
        timeout = INITIAL_NANNY_TIMEOUT
        for attempt in range(1, NANNY_REQUEST_RETRY_COUNT+1):
            try:
                response = await session.get(NANNY_INSTANCES_URL.format(service=service))
                if response.ok:
                    return _get_host_urls(await response.text())
                elif response.status == 404 and json.loads(await response.text()).get('error', '') == 'Not found':
                    raise ValueError(
                        f'No current instances cached for service {service}')
                else:
                    print(
                        f'status_code:{response.status} for service {service}\n{json.loads(await response.text()).get("msg", "")}')
                    return None
            except:
                await asyncio.sleep(timeout)
                timeout *= RETRY_COEFFICIENT
                print(
                    f'Bad request for service: {service} attempt: {attempt} retry in {timeout:.4f} sec')
    return None


def _get_hostnames(request_args: RequestArguments):
    futures = [_get_instances(service) for service in request_args.services]

    loop = asyncio.new_event_loop()
    done, pending = loop.run_until_complete(asyncio.wait(futures))
    loop.close()
    urls = []
    for location in done:
        if location.result():
            urls.extend(location.result())
    return urls


async def _make_request(url, request_args):
    async with aiohttp.ClientSession() as session:
        timeout = INITIAL_HOST_TIMEOUT
        for attempt in range(1, HOST_REQUEST_RETRY_COUNT+1):
            host_url = _build_url(url, request_args)
            try:
                response = await session.get(host_url)
                if response.ok:
                    return url, response.status, None
                return url, response.status, 'bad response'
            except Exception as ex:
                if attempt == HOST_REQUEST_RETRY_COUNT:
                    return url, 'exception during get request to host', ex
                print(
                    f'Bad request for host: {host_url} attempt: {attempt} retry in {timeout:.4f} sec')
                await asyncio.sleep(timeout)
                timeout *= RETRY_COEFFICIENT


def perform_requests(cmd_args):
    request_args = RequestArguments(cmd_args)
    hostnames = []
    if cmd_args.host:
        hostnames = [_update_url_to_admin(url)
                     for url in cmd_args.host.split(',')]
    else:
        hostnames = _get_hostnames(request_args)

    done = []
    loop = asyncio.new_event_loop()

    if cmd_args.apply_to_all:
        futures = [_make_request(url, request_args) for url in hostnames]
        done, pending = loop.run_until_complete(asyncio.wait(futures))
    else:
        for batch_start in range(0, len(hostnames), cmd_args.batch_size):
            futures = [_make_request(
                url, request_args) for url in hostnames[batch_start:batch_start+cmd_args.batch_size]]
            cur_done, pending = loop.run_until_complete(asyncio.wait(futures))
            done.extend(cur_done)
            if batch_start + cmd_args.batch_size >= len(hostnames):
                break
            cmd = ''
            continue_flag = True
            while not cmd:
                cmd = input('input continue (c) / stop (s): ')
                if cmd == 'continue' or cmd == 'c':
                    break
                elif cmd == 'stop' or cmd == 's':
                    continue_flag = False
                    break
                else:
                    print('unknown command, please retry')
                    cmd = ''
            if not continue_flag:
                break

    loop.close()

    failed_instances = [x.result()
                        for x in filter(lambda x: x.result()[-1], done)]
    return failed_instances


def main():
    args = WsproxyArgparser().parse_args()
    print(*perform_requests(args), sep='\n')


if __name__ == '__main__':
    main()
