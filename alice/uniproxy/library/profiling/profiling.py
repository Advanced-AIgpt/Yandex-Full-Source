import yappi
import tornado.web


# NOTE:
# These handlers start/stop profiling of only one process. Since Uniproxy is usually forked it
# may take few attempts to get into process being profiled and get results. Hence it's better
# not to fork Uniproxy for profiling purposes.


class StartProfilingHandler(tornado.web.RequestHandler):
    def get(self):
        clock_type = self.get_argument("clock", "cpu")
        if clock_type not in ("cpu", "wall"):
            raise tornado.web.HTTPError(status_code=400, reason="Invalid clock type")
        yappi.set_clock_type(clock_type)
        yappi.clear_stats()
        yappi.start()
        self.write("Yappi profiler started\n")


class StopProfilingHandler(tornado.web.RequestHandler):
    def get(self):
        if yappi.is_running():
            yappi.stop()
            with open("/logs/uniproxy_profile.callgrind", "wb") as f:
                yappi.get_func_stats().save(f.name, type="callgrind")
                self.write("Yappy profile is saved\n")
        else:
            self.set_status(404)
            self.write("Yappy profiler is not running\n")
