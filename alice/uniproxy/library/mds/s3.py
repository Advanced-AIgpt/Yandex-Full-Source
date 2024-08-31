import library.python.awssdk_async_extensions.lib.core as core

from botocore.exceptions import ClientError
import aiobotocore
import aiohttp
import io
import logging
import time
import tornado.gen


class WithLogger:
    def __init__(self, log):
        if log:
            self._log = log
        else:
            self._log = logging.getLogger(self.__class__.__name__)
            self._log.addHandler(logging.NullHandler())


class AsyncS3ProxyResolver(WithLogger):
    def __init__(
        self,
        receive_proxy_url: str,
        http_client_kwargs={},
        logger=None,
    ):
        super().__init__(logger)
        self.receive_proxy_url = receive_proxy_url

    async def get_proxy(self):
        async with aiohttp.ClientSession() as session:
            async with session.get(self.receive_proxy_url) as response:
                if response.status == 200:
                    body = await response.text()
                    self._log.debug("Resolved S3 proxy: {host}".format(host=body))
                    return body
                else:
                    self._log.warning(f"Can not resolve S3 proxy. Code: {response.status}")
                    return None


class AsyncS3Bucket(WithLogger):
    def __init__(
        self,
        s3_session,
        proxy_resolver,  # AsyncS3ProxyResolver or None
        endpoint_url: str,
        bucket_name: str,
        logger=None,
    ):
        super().__init__(logger)
        self.s3_session = s3_session
        self.proxy_resolver = proxy_resolver
        self.endpoint_url = endpoint_url
        self.bucket_name = bucket_name

        self.creation_ts = None

    async def update(self):
        now = time.time()
        if self.creation_ts is None or now - self.creation_ts > 30:  # update once in 30 seconds
            self.creation_ts = now
            await self._update()

    async def _update(self):
        self._log.debug("Updating proxy")

        if self.proxy_resolver:
            proxy = await self.proxy_resolver.get_proxy()
        else:
            proxy = None

        if proxy:
            session_config = aiobotocore.config.AioConfig(
                proxies={
                    "http": f"http://{proxy}:4080",
                },
            )
        else:
            session_config = None

        self.s3_resource_getter = self.s3_session.resource(
            "s3",
            endpoint_url=self.endpoint_url,
            config=session_config,
        )

    async def __aenter__(self):
        res = await self.s3_resource_getter.__aenter__()
        return await res.Bucket(self.bucket_name)

    async def __aexit__(self, exc_type, exc, tb):
        await self.s3_resource_getter.__aexit__(exc_type, exc, tb)


class S3Response:
    class S3ResponseCode:
        OK = 200
        NOT_FOUND = 404
        ERROR = 500

    def __init__(self, code=S3ResponseCode.OK, data=None, error=None):
        self.code = code
        self.data = data
        self.error = error


class AsyncS3Client(WithLogger):
    def __init__(
        self,
        tvm_client,  # PersonalTvmClient
        proxy_resolver,  # AsyncS3ProxyResolver or None
        endpoint_url: str,
        bucket_name: str,
        logger=None,
        retry_options={},
    ):
        super().__init__(logger)
        self.tvm_client = tvm_client
        self.proxy_resolver = proxy_resolver
        self.retry_options = retry_options
        self.endpoint_url = endpoint_url
        self.bucket_name = bucket_name

        self.s3_session = None
        self.bucket = None

    async def intialize(self):
        self._log.debug("Intializing S3 client")
        self.s3_session = await core.tvm2_session(self.tvm_client.get_service_ticket, self.tvm_client.self_alias)
        self.bucket = AsyncS3Bucket(
            s3_session=self.s3_session,
            proxy_resolver=self.proxy_resolver,
            endpoint_url=self.endpoint_url,
            bucket_name=self.bucket_name,
        )

    async def update_proxy(self):
        if self.bucket is None:
            await self.intialize()
        await self.bucket.update()

    async def _retry(self, func):
        attempts = self.retry_options.get("attempts", 3)
        sleep_duration = self.retry_options.get("sleep_duration", None)
        last_exception = None
        while attempts > 0:
            attempts -= 1
            try:
                return await func()
            except Exception as exception:
                last_exception = exception

            self._log.debug("s3 request failed with exception '{}'".format(str(last_exception)))

            if sleep_duration:
                tornado.gen.sleep(sleep_duration)

        return S3Response(
            error=last_exception,
            code=S3Response.S3ResponseCode.ERROR,
        )

    async def _try_get_object(self, key):
        with io.BytesIO() as fl:
            try:
                async with self.bucket as client:
                    await client.download_fileobj(key, fl)
            except ClientError as e:
                if e.response["Error"]["Code"] == "404":
                    return S3Response(
                        code=S3Response.S3ResponseCode.NOT_FOUND
                    )
                else:
                    raise

            return S3Response(
                data=fl.getvalue(),
                code=S3Response.S3ResponseCode.OK,
            )

    async def get_object(self, key):
        await self.update_proxy()

        async def func():
            return await self._try_get_object(key)

        return await self._retry(func)
