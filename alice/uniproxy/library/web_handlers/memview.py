import gc
import json
import random
import tempfile
import tornado.web
import tracemalloc
import objgraph
import os

from distutils.util import strtobool
from alice.uniproxy.library.logging import Logger


def list_with_weakproxy(lst):
    if not lst:
        return False

    for it in lst:
        try:
            if type(it).__name__ == 'weakproxy':
                return True
        except Exception:
            pass
    return False


class MemViewHandler(tornado.web.RequestHandler):
    snapshot = None

    def get(self):
        method = self.get_argument('method')
        if not method:
            self.set_status(400)
            self.write('expected "method" parameter')
            return

        Logger.get().info('handle method={}'.format(method))
        self.add_header('X-Uniproxy-Process', str(os.getpid()))
        pid = self.get_argument('pid', None)
        if pid and os.getpid() != int(pid):
            self.set_status(404)
            self.write('misshit pid=%d local pid=%d' % (pid, os.getpid()))
            return

        args = {}
        if method == 'most_common_types':
            self.set_args(args)
            lst = objgraph.most_common_types(**args)
            self.return_json(lst)
        elif method == 'count':
            self.set_args(args)
            lst = objgraph.count(self.get_argument('type'))
            self.return_json(lst)
        elif method == 'growth':
            self.set_args(args)
            lst = objgraph.growth(**args)
            self.return_json(lst)
        elif method == 'refs':
            self.set_args(args)
            obj = self.get_obj()
            with tempfile.NamedTemporaryFile(mode='rb', suffix='.svg') as fp:
                objgraph.show_refs(obj, filename=fp.name, **args)
                self.return_svg(fp)
        elif method == 'backrefs':
            self.set_args(args)
            obj = self.get_obj()
            with tempfile.NamedTemporaryFile(mode='rb', suffix='.svg') as fp:
                objgraph.show_backrefs(obj, filename=fp.name, **args)
                self.return_svg(fp)
        elif method == 'get_leaking_objects':
            self.return_json([str(obj) for obj in objgraph.get_leaking_objects()])
        elif method == 'most_fat_lists':
            self.set_args(args)
            result = []
            for slst in sorted(
                ((len(lst), id(lst)) for lst in objgraph.by_type('list')),
                key=lambda x: x[0],
                reverse=True,
            )[:args.get('limit', 100)]:
                result.append([slst[0], '0x{:x}'.format(slst[1])])
            self.return_json(result)
        elif method == 'most_fat_lists_with_weakproxy':
            self.set_args(args)
            result = []
            for slst in sorted(
                ((len(lst), id(lst)) for lst in objgraph.by_type('list') if list_with_weakproxy(lst)),
                key=lambda x: x[0],
                reverse=True,
            )[:args.get('limit', 100)]:
                result.append([slst[0], '0x{:x}'.format(slst[1])])
            self.return_json(result)
        elif method == 'most_fat_dicts':
            self.set_args(args)
            result = []
            for dict_info in sorted(
                ((len(d), id(d)) for d in objgraph.by_type('dict')),
                key=lambda x: x[0],
                reverse=True,
            )[:args.get('limit', 100)]:
                result.append([dict_info[0], '0x{:x}'.format(dict_info[1])])
            self.return_json(result)
        elif method == 'tracemalloc_start':
            if tracemalloc.is_tracing():
                self.write('already started')
                return
            nframe = int(self.get_argument('nframe', 1))
            tracemalloc.start(nframe)
            MemViewHandler.snapshot = tracemalloc.take_snapshot()
            self.write('started(nframe={})'.format(nframe))
        elif method == 'tracemalloc_stop':
            if not tracemalloc.is_tracing():
                self.write('already stopped')
                return
            MemViewHandler.snapshot = None
            tracemalloc.stop()
            self.write('stopped')
        elif method == 'tracemalloc_clear':
            tracemalloc.clear_traces()
            self.write('cleared')
        elif method == 'tracemalloc_diff':
            if not tracemalloc.is_tracing():
                self.write('tracemalloc stopped')
                return
            new_snapshot = tracemalloc.take_snapshot()
            key_type = self.get_argument('key_type', 'lineno')
            stat_diff = new_snapshot.compare_to(MemViewHandler.snapshot, key_type)
            MemViewHandler.snapshot = new_snapshot
            limit = int(self.get_argument('limit', '0'))
            result = []
            for n, rec in enumerate(stat_diff, 1):
                if limit and limit > n:
                    break
                frame = frame = rec.traceback[0]
                filename = os.sep.join(frame.filename.split(os.sep)[-2:])
                line = '{}:{}'.format(filename, frame.lineno)
                result.append(dict(
                    count=rec.count,
                    count_diff=rec.count_diff,
                    size=rec.size,
                    size_diff=rec.size_diff,
                    line=line,
                ))
            self.return_json(result)
        elif method == 'traced_memory':
            if not tracemalloc.is_tracing():
                self.set_status(403)
                self.write('tracemalloc stopped')
                return
            self.return_json(list(tracemalloc.get_traced_memory()))
        elif method == 'gc_collect':
            before = gc.get_count()
            generation = int(self.get_argument('generation', '2'))
            gc.collect(generation)
            after = gc.get_count()
            self.return_json(dict(before=before, after=after))
        elif method == 'gc_stats':
            self.return_json(gc.get_stats())
        elif method == 'gc_threshold':
            try:
                if self.get_argument('threshold0', None) is not None:
                    threshold0 = int(self.get_argument('threshold0'))
                    threshold1 = int(self.get_argument('threshold1'))
                    threshold2 = int(self.get_argument('threshold2'))
                    gc.set_threshold(threshold0, threshold1, threshold2)
                    self.write('gc threshold updated')
            except Exception as exc:
                self.write(str(exc))
            self.write('current_threshold: {}'.format(gc.get_threshold()))
        elif method == 'gc_garbage':
            result = []
            for obj in gc.garbage:
                try:
                    result.append([id(obj), str(obj)])
                except Exception as exc:
                    result.append([0, str(exc)])
            self.return_json(result)
        else:
            self.set_status(400)
            self.write('unsupported "method" parameter value == {}'.format(method))

    def set_args(self, args):
        limit = self.get_argument('limit', None)
        if limit is not None:
            args['limit'] = int(limit)
        shortnames = self.get_argument('shortnames', None)
        if shortnames is not None:
            args['shortnames'] = strtobool(shortnames)
        max_depth = self.get_argument('max_depth', None)
        if max_depth is not None:
            args['max_depth'] = int(max_depth)
        too_many = self.get_argument('too_many', None)
        if too_many is not None:
            args['too_many'] = int(too_many)

    def get_obj(self):
        addr = self.get_argument('addr', None)
        if addr:
            return objgraph.at(int(addr, 16))
        else:
            type_ = self.get_argument('type')
            return random.choice(objgraph.by_type(type_))

    def return_json(self, j):
        self.set_status(200)
        self.add_header('Content-Type', 'application/json')
        self.write(json.dumps(j, indent=4, separators=(',', ': ')))

    def return_svg(self, fp):
        self.add_header('Content-Type', 'image/svg+xml')
        self.write(fp.read())
