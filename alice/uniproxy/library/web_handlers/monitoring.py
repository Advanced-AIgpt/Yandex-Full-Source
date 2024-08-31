import tornado.escape
import os

from alice.uniproxy.library.common_handlers import CommonRequestHandler


class MonitoringHandler(CommonRequestHandler):
    unistat_handler_name = 'monitoring'

    def initialize(self, logfile, expected_oks=1, unistat=None):
        self.logfile = logfile
        self.expected_oks = expected_oks
        if unistat:
            self.unistat_handler_name = unistat

    def get(self, topic=None):
        if topic:
            self.logfile += topic

        try:
            if not os.path.exists(self.logfile):
                self.set_status(500)
                self.write("No monitoring log in %s" % (tornado.escape.url_escape(self.logfile),))
            else:
                with open(self.logfile) as logfile:
                    res = logfile.read()
                    if res.count("OK") != self.expected_oks:
                        self.set_status(503)
                    else:
                        self.set_status(200)
                    self.write(res)
        except Exception as exc:
            self.set_status(501)
            self.write("Exception in handler: %s" % (exc,))
