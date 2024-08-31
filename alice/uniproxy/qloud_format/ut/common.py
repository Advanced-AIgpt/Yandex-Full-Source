import yatest.common
import asyncio.subprocess
import socket


# APP_PATH = "../qloud_format"
APP_PATH = yatest.common.binary_path("alice/uniproxy/qloud_format/qloud_format")
HOST_FQDN = socket.getfqdn()
TIME_FMT = "%Y-%m-%dT%H:%M:%S%z"


def with_async_subprocess(program, *args):
    def decorator(func):
        def wrap():
            async def async_test_func():
                process = await asyncio.create_subprocess_exec(
                    program,
                    *args,
                    stdin=asyncio.subprocess.PIPE,
                    stdout=asyncio.subprocess.PIPE,
                    limit=1024*1024
                )
                try:
                    await func(process)
                    if process.returncode is None:
                        process.terminate()
                        assert 0 == await process.wait()
                finally:
                    if process.returncode is None:
                        process.kill()

            loop = asyncio.get_event_loop()
            loop.run_until_complete(async_test_func())

        return wrap

    return decorator


def with_qloud_format(*args):
    return with_async_subprocess(APP_PATH, *args)
