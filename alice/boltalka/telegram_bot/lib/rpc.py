from xmlrpc.server import SimpleXMLRPCServer
from alice.boltalka.telegram_bot.lib.module import Module


class RpcSourceServer:
    def __init__(self, host, port, modules):
        self.server = SimpleXMLRPCServer((host, port), allow_none=True)
        self.server.register_introspection_functions()
        self.modules = {module.__name__: module for module in modules}
        self.instances = {}

    def run(self):
        self.server.register_function(self.get_candidates)
        self.server.register_function(self.set_option)
        self.server.register_function(self.get_options)
        self.server.serve_forever()

    def get_module(self, args):
        source_type = args['source_type']
        if source_type not in self.instances:
            self.instances[source_type] = self.modules[source_type]()
        return self.instances[source_type]

    def get_candidates(self, args, context):
        return self.get_module(args).get_candidates(args, context)

    def set_option(self, args, option, value):
        self.get_module(args).set_option(args, option, value)

    def get_options(self, args):
        self.get_module(args).get_options(args)


class PhonySource(Module):
    class Options:
        message = 'hello'

    def get_candidates(self, context):
        return [dict(text=self.message, relevance=0)]


if __name__ == '__main__':
    server = RpcSourceServer('localhost', 5555, [PhonySource])
    server.run()
