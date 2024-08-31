from aiohttp import web
import asyncio
import logging
import aioredis
from typing import Dict, Callable, AsyncIterator, List

from alice.paskills.penguinarium.routes import setup_routes
from alice.paskills.penguinarium.ml.embedder import CachedDssmEmbedder
from alice.paskills.penguinarium.ml.index import SklearnIndex, ModelFactory
from alice.paskills.penguinarium.storages.ydb_utils import YdbManager
from alice.paskills.penguinarium.storages.nodes import (
    BaseNodesStorage, YdbNodesStorage
)
from alice.paskills.penguinarium.ml.intent_resolver import DssmKnnIntentResolver
from alice.paskills.penguinarium.util.metrics import sensors, RedisMetricsStorage
from alice.paskills.penguinarium.views.middleware import metrics_middleware
from alice.paskills.penguinarium.storages.graph import YdbGraphsStorage

logger = logging.getLogger(__name__)
CleanupCallable = Callable[[web.Application], AsyncIterator[None]]


def redis(config: Dict) -> CleanupCallable:
    async def _redis(app: web.Application) -> AsyncIterator[None]:
        redis_cfg = config['redis']
        app['redis'] = await aioredis.create_redis_pool(
            redis_cfg['address'],
            maxsize=redis_cfg['maxsize']
        )
        sensors.setup(RedisMetricsStorage(app['redis'], config['metrics']))

        yield

        redis = app['redis']
        redis.close()
        await redis.wait_closed()

    return _redis


async def do_cache_warm_up(
    storage: BaseNodesStorage,
    nodes_idx: List[str]
) -> None:
    try:
        await asyncio.gather(*(
            storage.get(node_id, use_cache=False)
            for node_id in nodes_idx
        ))
        await sensors.inc_counter('cache_warm_up')
        await sensors.storage.flush()
    except Exception:
        logging.exception('Exception at do_cache_warm_up')


async def periodic_cache_warm_up(
    storage: BaseNodesStorage,
    warm_up_cfg: Dict
) -> None:
    while True:
        try:
            await asyncio.sleep(warm_up_cfg['sleep_time'])
            await do_cache_warm_up(storage, warm_up_cfg['nodes_idx'])

        except asyncio.CancelledError:
            logging.info('Cache warm up canceled')
            break


def cache_warm_up(config: Dict) -> CleanupCallable:
    async def _cache_warm_up(app: web.Application) -> AsyncIterator[None]:
        warm_up_cfg = config['nodes_storage']['warm_up']
        await do_cache_warm_up(app['nodes_storage'], warm_up_cfg['nodes_idx'])

        warm_up_coro = periodic_cache_warm_up(app['nodes_storage'], warm_up_cfg)
        app['cache_warm_up'] = asyncio.create_task(warm_up_coro)

        yield

        app['cache_warm_up'].cancel()
        await app['cache_warm_up']

    return _cache_warm_up


def ydb(config: Dict) -> CleanupCallable:
    async def _ydb(app: web.Application) -> AsyncIterator[None]:
        ydb_cfg = config['ydb']
        app['ydb_manager'] = YdbManager(
            endpoint=ydb_cfg['endpoint'],
            database=ydb_cfg['database'],
            root_folder=ydb_cfg['root_folder'],
            connect_params=ydb_cfg['connect_params'],
            timeout=ydb_cfg['timeout'],
            max_retries=ydb_cfg['max_retries'],
        )

        yield

        logger.info('Stopping YDB connection')
        ydb_manager = app['ydb_manager']
        ydb_manager.driver.stop()
        ydb_manager.session_pool.stop()

    return _ydb


def nodes_storage(config: Dict) -> CleanupCallable:
    async def _nodes_storage(app: web.Application) -> AsyncIterator[None]:
        app['nodes_storage'] = YdbNodesStorage(
            ydb_manager=app['ydb_manager'],
            model_factory=app['model_factory'],
            cache_size=config['nodes_storage']['cache_size'],
            ttl=config['nodes_storage']['ttl'],
        )
        yield

    return _nodes_storage


def graph_storage(config: Dict) -> CleanupCallable:
    async def _graph_storage(app: web.Application) -> AsyncIterator[None]:
        app['graph_storage'] = YdbGraphsStorage(
            ydb_manager=app['ydb_manager'],
            nodes_storage=app['nodes_storage']
        )
        yield

    return _graph_storage


def init_app(config: Dict) -> web.Application:
    app = web.Application(
        middlewares=[metrics_middleware],
        client_max_size=10 * 1024 * 1024
    )

    app['config'] = config

    dssm_cfg = config['dssm']
    app['embedder'] = CachedDssmEmbedder(
        path=dssm_cfg['path'],
        input_name=dssm_cfg['input_name'],
        output_name=dssm_cfg['output_name'],
        empty_inputs=dssm_cfg['empty_inputs'],
        cache_size=dssm_cfg['cache_size']
    )

    model_cfg = config['model']
    app['model_factory'] = ModelFactory(
        SklearnIndex,
        thresh=model_cfg['thresh'],
        dist_thresh_rel=model_cfg['dist_thresh_rel'],
        metric=model_cfg['metric'],
        n_neighbors=model_cfg['n_neighbors'],
        p=model_cfg['p']
    )

    app['intent_resolver'] = DssmKnnIntentResolver(
        embedder=app['embedder'],
        model_factory=app['model_factory']
    )

    setup_routes(app)

    if config['redis'].get('turned_on', True):
        app.cleanup_ctx.append(redis(config))
    app.cleanup_ctx.append(ydb(config))
    app.cleanup_ctx.append(nodes_storage(config))
    app.cleanup_ctx.append(cache_warm_up(config))
    app.cleanup_ctx.append(graph_storage(config))

    return app
