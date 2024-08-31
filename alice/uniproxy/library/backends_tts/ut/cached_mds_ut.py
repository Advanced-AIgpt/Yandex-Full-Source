import tornado.gen
from contextlib import contextmanager
from alice.uniproxy.library.testing import run_async
from alice.uniproxy.library.testing.mocks import MdsServerMock
from alice.uniproxy.library.testing.config_patch import ConfigPatch
from alice.uniproxy.library.backends_tts.cached_mds import get_from_cached_mds, reset_cached_mds
from alice.uniproxy.library.global_counter import GlobalCounter


@contextmanager
def mds_server(content):
    with MdsServerMock(content) as srv:
        with ConfigPatch({
            "tts": {
                "s3_url": f"{srv.url}/%s",
                "s3_cache_ttl": 5,
                "s3_cache_maxsize": 200
            }
        }):
            reset_cached_mds()
            GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.set(0)
            yield srv


@run_async()
async def test_updates_on_ttl():
    content = {
        "/aaa": "AAA",
        "/bbb": "BBB"
    }

    with mds_server(content) as mds:
        # get initial valus from MDS
        assert (await get_from_cached_mds("aaa")) == b"AAA"
        assert mds.requests == 1
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 3

        assert (await get_from_cached_mds("bbb")) == b"BBB"
        assert mds.requests == 2
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 6

        # data is updated in MDS
        content["/aaa"] = "AAA-2"
        content["/bbb"] = "BBB-2"

        # use values from cache
        assert (await get_from_cached_mds("aaa")) == b"AAA"
        assert (await get_from_cached_mds("bbb")) == b"BBB"
        assert mds.requests == 2
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 6

        # sleep over TTL
        await tornado.gen.sleep(6)

        # cached values are still in use (updates were initiated in background)
        assert (await get_from_cached_mds("aaa")) == b"AAA"
        assert (await get_from_cached_mds("bbb")) == b"BBB"

        # new values must be obtained
        await tornado.gen.sleep(2)
        assert (await get_from_cached_mds("aaa")) == b"AAA-2"
        assert (await get_from_cached_mds("bbb")) == b"BBB-2"
        assert mds.requests == 4
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 10


@run_async()
async def test_evictions_on_capacity():
    content = {
        "/small-a": "AAA",
        "/small-b": "BBB",
        "/big-a": "A" * 100,
        "/big-b": "B" * 100
    }

    with mds_server(content) as mds:
        # populate cache with 3 items
        assert (await get_from_cached_mds("small-a")) == b"AAA"
        assert mds.requests == 1
        assert (await get_from_cached_mds("small-b")) == b"BBB"
        assert mds.requests == 2
        assert (await get_from_cached_mds("big-a")) == b"A" * 100
        assert mds.requests == 3
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 106

        # check if all 3 items are cached (no requests to MDS)
        assert (await get_from_cached_mds("small-a")) == b"AAA"
        assert (await get_from_cached_mds("small-b")) == b"BBB"
        assert (await get_from_cached_mds("big-a")) == b"A" * 100
        assert mds.requests == 3
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 106

        # request for 'big-b' - exceed cache's capacity so LRU items must be evicted
        assert (await get_from_cached_mds("big-b")) == b"B" * 100
        assert mds.requests == 4
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 200

        # here only 'big-a' and 'big-b' are in cache
        assert (await get_from_cached_mds("big-a")) == b"A" * 100
        assert (await get_from_cached_mds("big-b")) == b"B" * 100
        assert mds.requests == 4
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 200

        # request for small items - LRU 'big-a' must be evicted
        assert (await get_from_cached_mds("small-a")) == b"AAA"
        assert mds.requests == 5
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 103

        assert (await get_from_cached_mds("small-b")) == b"BBB"
        assert mds.requests == 6
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 106

        # 'big-b' is still in cache
        assert (await get_from_cached_mds("big-b")) == b"B" * 100
        assert mds.requests == 6
        assert GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.value() == 106
