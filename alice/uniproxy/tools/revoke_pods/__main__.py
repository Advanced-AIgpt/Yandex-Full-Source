import argparse
import os
import asyncio
import aiohttp

REQUEST_URL = 'https://yp-lite-ui.nanny.yandex-team.ru/api/yplite/pod-sets/RequestPodEviction/'
NANNY_TOKEN = os.environ.get('NANNY_TOKEN')
CHUNK_SIZE = 10

HEADERS = {'Authorization': f'OAuth {NANNY_TOKEN}',
           'Content-Type': 'application/json'}


def make_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', help='File with pods list')
    return parser


async def evict(filename):
    chunked_pods = read_and_break_to_parts(filename)
    async with aiohttp.ClientSession(raise_for_status=False, headers=HEADERS) as session:
        for chunk in chunked_pods:
            await send_revoke_requests(chunk, session)


def read_and_break_to_parts(filename):
    with open(filename) as f:
        data = f.readlines()
    if len(data) <= CHUNK_SIZE:
        return [data]
    return [data[i:i + CHUNK_SIZE] for i in range(0, len(data), CHUNK_SIZE)]


async def send_revoke_requests(chunk, session):
    tasks = []
    for pod in chunk:
        tasks.append(send_evict_requests(pod, session))
    await asyncio.gather(*tasks)


async def send_evict_requests(pod, session):
    pod_id, dc = await get_pod_id_and_dc(pod)
    if pod_id is None:
        return
    async with session.post(REQUEST_URL, json={"cluster": dc.upper(), "pod_id": pod_id}) as resp:
        if resp.status != 200:
            print(f"Eviction error: {pod_id}: {await resp.json()}")
            return
    print(f'OK: {pod_id}')


async def get_pod_id_and_dc(pod):
    pod = pod.strip()
    if not pod.endswith('yp-c.yandex.net'):
        print(f'Bad pod name: {pod}')
        return None, None
    split_pod = pod.split('.')
    return split_pod[0], split_pod[1]


if __name__ == '__main__':
    parser = make_parser()
    arg = parser.parse_args()
    asyncio.run(evict(arg.filename))
