import time
import math
import inspect

import tornado.gen
from tornado.queues import Queue
from tornado.ioloop import IOLoop

from alice.uniproxy.library.logging import Logger
from voicetech.library.aproc.py.merger import AudioMerger


def _divisible_ceil(val, divisor):
    return divisor * math.ceil(val / divisor)


def _divisible_floor(val, divisor):
    return divisor * (val // divisor)


class ComplexOpusStream:
    def __init__(self, writer, background_audio=None, jitter_duration=5, min_output_duration=0.02, sample_rate=48000):
        self._log = Logger.get('uniproxy.ComplexOpusStream')
        self._writer = writer
        self._background_audio = b"" if (background_audio is None) else background_audio
        self._sample_rate = sample_rate
        self._jitter_buff = jitter_duration
        self._min_output_samples = min_output_duration * self._sample_rate

        self._proc = None

        self._jitter_buff_end = 0
        self._incoming_chunks = Queue()
        self._eos = False
        self._finished = False

        IOLoop.current().spawn_callback(self.run)

    def push_chunk(self, data):
        self._incoming_chunks.put_nowait(data)

    def seconds_to_jitter(self):
        "Number of seconds needed to fill jitter buffer"
        jitter_duration = max(0, self._jitter_buff_end - time.monotonic())
        return self._jitter_buff - jitter_duration

    def _write(self, data, duration):
        now = time.monotonic()
        if self._jitter_buff_end <= now:
            self._jitter_buff_end = now + duration
        else:
            self._jitter_buff_end += duration
        self._writer(data)

    def _consume_chunk(self, chunk):
        if chunk is not None:
            self._proc.push_chunk(chunk)
        else:
            self._eos = True
        self._incoming_chunks.task_done()

    async def _get_samples(self, samples):
        ready_samples = self._proc.ready_samples_to_merge
        while (ready_samples < samples) and (not self._eos):
            if self._incoming_chunks.empty():
                samples = self._min_output_samples  # wait up to minimal number os samples
                self._consume_chunk(await self._incoming_chunks.get())

            # consume all we can without awaiting
            while (not self._incoming_chunks.empty()):
                self._consume_chunk(self._incoming_chunks.get_nowait())

            ready_samples = self._proc.ready_samples_to_merge

        return ready_samples

    async def run(self):
        if inspect.isawaitable(self._background_audio):
            self._background_audio = (await self._background_audio) or b""

        self._proc = AudioMerger(self._background_audio)

        ready_samples = 0
        finish = False

        while not finish:
            desired_seconds = self.seconds_to_jitter()
            if desired_seconds < 0:
                await tornado.gen.sleep(-desired_seconds)
                desired_seconds = self.seconds_to_jitter()  # desired_seconds >= 0

            # calculated desired number of samples aligned with minimal one
            if desired_seconds == 0:
                samples = self._min_output_samples
            else:
                samples = desired_seconds * self._sample_rate
                samples = _divisible_ceil(samples, self._min_output_samples)

            if not self._eos:
                # obtain from `_min_output_samples` up to `samples`
                ready_samples = await self._get_samples(samples)

            # number of samples must not exceed available
            if self._eos:
                if ready_samples <= samples:
                    samples = ready_samples
                    finish = True
            elif ready_samples < samples:
                samples = _divisible_floor(ready_samples, self._min_output_samples)

            ready_samples -= samples
            duration = samples / self._sample_rate

            self._log.debug(
                f"desired_seconds={desired_seconds} "
                f"duration={duration} "
                f"samples={samples} "
                f"ready_samples={ready_samples} "
            )

            data = self._proc.pop_merged(samples, finish)
            self._write(data, duration)

        self._writer(None)  # means end-of-output
