import asyncio
import logging
import signal

from .runner import parse_known_args, run, run_unified_agent

__all__ = ['parse_known_args', 'run', 'run_unified_agent']

logging.getLogger(__name__).addHandler(logging.NullHandler())


async def _shutdown(sig, loop):
    logging.info(f'caught {sig}')
    tasks = [_ for _ in asyncio.all_tasks() if _ is not asyncio.current_task()]
    for i in range(len(tasks) - 1, -1, -1):
        tasks[i].cancel()
        await asyncio.sleep(2)
    results = await asyncio.gather(*tasks, return_exceptions=True)
    logging.info(f'processes finised, results: {results}')
    loop.stop()


def async_loop(func):
    async def wrapper(*args, **kwargs):
        loop = asyncio.get_event_loop()
        for signame in ('SIGINT', 'SIGTERM'):
            loop.add_signal_handler(
                getattr(signal, signame),
                lambda signame=signame: asyncio.create_task(_shutdown(signame, loop)),
            )

        aws = await func(*args, **kwargs)
        try:
            result = await asyncio.gather(*aws)
            logging.info(f'results: {result}')
        except BaseException as e:
            logging.error(e)
            loop = asyncio.get_running_loop()
            if loop is not None:
                shut = await _shutdown(f'Exception {str(e)}', loop)
                if shut is not None:
                    shut.wait()

    return wrapper
