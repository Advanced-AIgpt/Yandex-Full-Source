from tornado.ioloop import IOLoop
from tornado.queues import Queue

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.settings import config
from .opus_stream import RealtimeOpusStream
from .complex_opus_stream import ComplexOpusStream
from .cached_mds import get_from_cached_mds


class Streamer:  # base common streamer class
    def __init__(self, system, cfg, ref_stream_id):
        self.system = system
        self.cfg = cfg
        if ref_stream_id is None:
            GlobalCounter.U2_SYSTEM_STREAM_ID_SUMM.increment()
            self.stream_id = system.next_stream_id()
        else:
            GlobalCounter.U2_REF_STREAM_ID_SUMM.increment()
            self.stream_id = ref_stream_id
        self.closed = False
        self.on_close = []

    def add_data(self, data):
        'add data from tts_stream using this method'
        if self.closed:
            return

        self.add_data_impl(data)

    def add_data_impl(self, data):
        raise Exception('not impl. Streamer.add_data_impl')

    def next_chunk(self, size):
        'handler client reqeust for next chunk for LazyStreamer'
        self.system.WARN('called Streamer next_chunk for not LazyStreamer')

    def _write(self, data: bytes):
        if self.closed:
            return

        try:
            if data is None:
                self.close()
            else:
                self.system.write_data(self.stream_id, data)
        except Exception as exc:
            self.system.EXC(exc)

    def on_eof(self):
        if self.on_eof_callback:
            self.on_eof_callback()
        self.close()

    def finalize(self, callback_on_close):
        """ soft stopping - wait streaming buffered data
            return True if all data already sended, else return False & use callback later
        """
        if self.closed:
            return True

        if self.finalize_impl():
            self.close()
            return True

        if callback_on_close:
            self.on_close.append(callback_on_close)
        return False

    def finalize_impl(self):
        raise Exception('not impl. Streamer.finalize_impl')

    def close(self):
        'immediately stop streamer (send streamcontrol before stopping)'
        if self.closed:
            return False

        self.system.close_stream(self.stream_id)
        self.abort()

    def abort(self):
        'silently/immediately stop streamer (without sending streamcontrol)'
        if self.closed:
            return False  # not need anymore

        self.closed = True
        if self.on_close:
            for callback in self.on_close:
                try:
                    callback()
                except Exception:
                    self.system.EXC('exception from callback usage from Streamer')
            self.on_close = []
        return True


class FakeStreamer(Streamer):
    'simple proxy data from tts_stream to client'
    def add_data_impl(self, data: bytes):
        self._write(data)

    def finalize_impl(self):
        self.close()
        return True


class LazyStreamer(Streamer):
    def __init__(self, system, cfg, ref_stream_id):
        super().__init__(system, cfg, ref_stream_id)
        self.data_queue = []
        self.need_next_chunk = False

    def add_data_impl(self, data: bytes):
        if self.need_next_chunk:
            self.need_next_chunk = False
            if data is None:
                self.close()
            elif len(data):
                self._write(data)
            # else has empty chunk - ignore it
        else:
            self.data_queue.append(data)

    def next_chunk(self, size=None):
        need_bytes = size if size else 1000000
        chunk = b''
        while self.data_queue:
            data = self.data_queue[0]
            if data is None:
                if len(chunk):
                    self._write(chunk)
                self._write(None)
                del self.data_queue[0]
                self.close()
                return

            if len(data) > need_bytes:
                chunk += data[:need_bytes]
                self.data_queue[0] = data[need_bytes:]
                break

            chunk += data
            need_bytes -= len(data)
            del self.data_queue[0]
            if not need_bytes:
                break

        if len(chunk):
            self._write(chunk)
        else:
            self.need_next_chunk = True

    def finalize_impl(self):
        if self.data_queue:
            self.add_data(None)
            return False

        return True


class OpusStreamer(Streamer):
    def __init__(self, system, cfg, ref_stream_id, opus_cfg):
        super().__init__(system, cfg, ref_stream_id)
        self.opus_data_queue = Queue()
        self.realtime_opus_stream_buffer = opus_cfg.get("buffer", 5)
        self.realtime_opus_stream = None
        self.left_opus_data = b''

    def add_data_impl(self, data: bytes):
        if self.realtime_opus_stream is None:
            self.system.DLOG('create RealtimeOpusStream', rt_log=self.system.rt_log)
            self.realtime_opus_stream = RealtimeOpusStream(self, self, self.realtime_opus_stream_buffer)
            IOLoop.current().spawn_callback(self.process)
        self.opus_data_queue.put_nowait(data)

    async def process(self):
        """ we need closing unisystem audio stream on finish, so override process()
            (also catch & log exceptions here)
        """
        try:
            await self.realtime_opus_stream.process()
        except Exception as exc:
            self.system.EXC(exc)
        try:
            self.close()
        except Exception:
            pass

    async def read(self, size):
        """ impl. RealtimeOpusStream input stream
        """
        if self.closed:
            return None

        if len(self.left_opus_data):
            if len(self.left_opus_data) >= size:
                data = self.left_opus_data[:size]
                self.left_opus_data = self.left_opus_data[size:]
                return data

            data = self.left_opus_data
            self.left_opus_data = b''
            size -= len(data)
        else:
            data = b''

        while not self.closed:
            next_chunk = await self.opus_data_queue.get()
            if next_chunk is None:
                return None

            if len(next_chunk) >= size:
                data += next_chunk[:size]
                self.left_opus_data = next_chunk[size:]
                return data

            data += next_chunk
            size -= len(next_chunk)
        return None

    async def write(self, data):
        """ impl. RealtimeOpusStream output stream
        """
        if self.closed:
            return

        # UniSystem.write_data() impl. now use not synchronized (ignored) future
        self.system.DLOG('RealtimeOpusStream send next chunk size={}'.format(len(data)), rt_log=self.system.rt_log)
        self._write(data)

    def finalize_impl(self):
        if not self.realtime_opus_stream:
            return True

        self.add_data(None)  # send eof marker
        return False

    def abort(self):
        if not super().abort():
            return
        if self.opus_data_queue is not None:
            self.opus_data_queue.put_nowait(None)


class RealtimeStreamer(Streamer):
    def __init__(self, system, cfg, ref_stream_id):
        super().__init__(system, cfg, ref_stream_id)
        self.data_queue = []
        self.min_chunk_size = self.cfg.get("min_chunk_size", 2000)
        self.interval = self.cfg.get("interval", 0.15)

        self.repeat_writer = None
        self.on_eof_callback = None

    def add_data(self, data: bytes):
        if self.interval == 0:
            self._write(data)
            return

        self.data_queue.append(data)

        if self.repeat_writer is not None:  # already sending, nothing need todo
            return

        self.write_next()  # init repeat writing

    def write_next(self):
        if not self.data_queue or self.closed:
            self.repeat_writer = None
            return

        eof = False
        try:
            data = self.data_queue[0]
            if data is None:
                eof = True
                next_frame_pos = -1
            else:
                # select sended data chunk size
                next_frame_pos = 0
                while next_frame_pos < self.min_chunk_size and next_frame_pos != -1:
                    next_frame_pos = data.find(b'OggS', next_frame_pos + 1)

            if next_frame_pos == -1:
                # send entire data
                del self.data_queue[0]
                self._write(data)
            else:
                self.data_queue[0] = data[next_frame_pos:]
                self._write(data[:next_frame_pos])

            if self.data_queue and not self.closed:
                self.repeat_writer = IOLoop.current().call_later(self.interval, self.write_next)
                return

        except Exception:
            self.system.EXC('RealtimeStreamer exception')
            eof = True
        self.repeat_writer = None
        if eof:
            self.close()

    def abort(self):
        if not super().abort():
            return

        if self.repeat_writer:
            IOLoop.current().remove_timeout(self.repeat_writer)
            self.repeat_writer = None

    def finalize_impl(self):
        if not self.data_queue:
            return True

        self.add_data(None)
        return False


# -------------------------------------------------------------------------------------------------
class RealtimeOpusWithBackgroundStreamer(Streamer):
    def __init__(self, system, cfg, ref_stream_id, opus_cfg):
        super().__init__(system, cfg, ref_stream_id)

        background = cfg.get("background")
        self.system.DLOG(f"create RealtimeOpusWithBackgroundStreamer, background='{background}'", rt_log=self.system.rt_log)

        self._stream = ComplexOpusStream(
            self._write,
            background_audio=get_from_cached_mds(background, nothrow=True),  # no background if an error occurs
            jitter_duration=opus_cfg.get("buffer", 5)
        )

    def add_data_impl(self, data: bytes):
        self._stream.push_chunk(data)

    def finalize_impl(self):
        self._stream.push_chunk(None)
        return False

    def abort(self):
        if super().abort():
            self._stream.push_chunk(None)


# -------------------------------------------------------------------------------------------------
def create_streamer(system, cfg, ref_stream_id):
    # ensure config exist and based on system config
    streamer_config = dict(config["streamer"])
    if cfg is not None:
        streamer_config.update(cfg)
    cfg = streamer_config

    if not cfg.get("enabled", False):
        system.DLOG('create FakeStreamer')
        return FakeStreamer(system, cfg, ref_stream_id)

    if cfg.get("sync_chunker", False):
        system.DLOG('create LazyStreamer')
        return LazyStreamer(system, cfg, ref_stream_id)

    opus_cfg = cfg.get("opus", {})
    mime = cfg.get("mime", "audio/opus")
    if opus_cfg.get("enabled", False):
        if "opus" in mime:
            if "background" in cfg:
                system.DLOG('create RealtimeOpusWithBackgroundStreamer')
                return RealtimeOpusWithBackgroundStreamer(system, cfg, ref_stream_id, opus_cfg)
            system.DLOG('create OpusStreamer')
            return OpusStreamer(system, cfg, ref_stream_id, opus_cfg)
        else:
            system.WARN('can not create OpusStreamer for non-opus stream: mime={}'.format(mime))

    system.DLOG('create RealtimeStreamer')
    return RealtimeStreamer(system, cfg, ref_stream_id)
