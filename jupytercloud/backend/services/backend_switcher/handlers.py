from jupyterhub.utils import url_path_join
from tornado import web


started = False
dc = ''


class ReadyHandler(web.RequestHandler):
    async def get(self):
        if started:
            self.set_status(200)
            self.write('Ready!')
        else:
            self.set_status(318)
            self.write("I'm a teapot!")


class DCHandler(web.RequestHandler):
    async def get(self):
        if started:
            self.set_status(200)
            self.write(dc)


def get_handlers(prefix):
    paths = [
        (r'/ready', ReadyHandler),
        (r'/', DCHandler),
    ]

    return [(url_path_join(prefix, path), handler) for path, handler in paths]
