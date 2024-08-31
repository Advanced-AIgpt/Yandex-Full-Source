from alice.uniproxy.library.common_handlers import CommonRequestHandler


try:
    from library.python.svn_version import svn_revision
except:
    def svn_revision():
        return 0


class RevisionHandler(CommonRequestHandler):
    unistat_handler_name = 'revision'

    def get(self):
        self.add_header('Content-Type', 'text/plain')
        self.finish(str(svn_revision()))
