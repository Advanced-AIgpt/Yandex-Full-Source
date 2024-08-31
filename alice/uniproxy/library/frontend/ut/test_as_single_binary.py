import logging
import tornado.web
import tornado.httpclient
from alice.uniproxy.library.frontend import RESOURCES_ROOT, FileHandler, TemplateLoader
from alice.uniproxy.library.global_counter import GlobalCounter
import library.python.resource


GlobalCounter.init()


class HttpServer:
    def __init__(self):
        self._app = tornado.web.Application([
            (r'/(.*)', FileHandler, {"path": RESOURCES_ROOT})
        ])
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self.port = None

    def start(self):
        for port in range(30000, 0x10000):
            try:
                self._srv.listen(port)
                self.port = port
                return
            except OSError:
                pass
        raise RuntimeError("Couldn't find free port for HttpServer")

    def stop(self):
        self._srv.stop()

    def __enter__(self, *args, **kwargs):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    @property
    def url(self):
        return "http://localhost:{}".format(self.port)


def run_async(func):
    import tornado.ioloop

    def test_async_wrap():
        tornado.ioloop.IOLoop.current().run_sync(func)

    return test_async_wrap


# ====================================================================================================================
@run_async
async def test_simple():
    files = (  # some arbitrary files
        "analyser.js",
        "demo.html",
        "demo.js",
        "mic0.png",
        "settings.js",
        "ttsdemo.html",
        "unidemo.html",
        "web_push.js",
        "highlight/highlight.pack.js",
        "highlight/styles/brown-paper.css",
        "highlight/styles/brown-papersq.png",
        "webspeechkit/README.md",
        "webspeechkit/ui/equalizer.js"
    )
    with HttpServer() as srv:
        client = tornado.httpclient.AsyncHTTPClient()
        for f in files:
            logging.debug("request for {}...".format(f))
            response = await client.fetch(srv.url + "/" + f)
            assert response.code == 200
            assert response.body == library.python.resource.find("/frontend/" + f)


# ====================================================================================================================
@run_async
async def test_non_existing():
    GlobalCounter.HANDLER_COMMON_REQS_SUMM.set(0)
    with HttpServer() as srv:
        client = tornado.httpclient.AsyncHTTPClient()
        response = await client.fetch(srv.url + "/non-existing-file.html", raise_error=False)
        assert response.code == 404
        assert GlobalCounter.HANDLER_COMMON_REQS_SUMM.value() == 1


# ====================================================================================================================
def test_template_loader():
    loader = TemplateLoader(RESOURCES_ROOT)
    result = loader.load("ttsdemo.html").generate(
        voice="oksana",
        speed="",
        emotion="",
        text=u"Тестирование синтеза речи.",
        lang="ru",
        apikey="some-key",
        tr=lambda x, y: x
    ).decode("utf-8")

    assert "oksana" in result
    assert u"Тестирование синтеза речи." in result
