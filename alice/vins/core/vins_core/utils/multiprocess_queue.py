# coding: utf-8
from __future__ import absolute_import

from multiprocessing import Process, Queue as mpQueue
from Queue import Queue as plainQueue
from threading import Thread


class Queue(object):
    def __init__(self, maxsize, num_iterations):
        self._mpq = mpQueue(maxsize=maxsize)
        self._qq = plainQueue(maxsize=maxsize)

        self.maxsize = maxsize
        self.num_iterations = num_iterations

        self.steal_daemon()

    def put(self, item):
        self._mpq.put(item)

    def get(self):
        return self._qq.get()

    def stop(self):
        self._stealer.join()

    def steal_daemon(self):
        def steal(srcq, dstq, num_iterations):
            for _ in xrange(num_iterations):
                obj = srcq.get()
                dstq.put(obj)

        self._stealer = Thread(target=steal, args=(self._mpq, self._qq, self.num_iterations))
        self._stealer.start()


class MultiprocessIterator:
    def __init__(self, make_gen, njobs, maxsize, num_iterations):
        self.make_gen = make_gen
        self.njobs = njobs
        self.maxsize = maxsize
        self.num_iterations = num_iterations

    def __iter__(self):
        self.queue = Queue(maxsize=self.maxsize, num_iterations=self.num_iterations)
        self.ps = []
        self.iterations_passed = 0

        def worker(q, gen):
            for item in gen:
                q.put(item)

        for _ in xrange(self.njobs):
            p = Process(target=worker, args=(self.queue, self.make_gen()))
            p.daemon = True
            p.start()
            self.ps.append(p)

        return self

    def next(self):
        if self.iterations_passed == self.num_iterations:
            raise StopIteration

        result = self.queue.get()
        self.iterations_passed += 1

        if self.iterations_passed == self.num_iterations:
            for p in self.ps:
                p.terminate()
            self.queue.stop()

        return result
