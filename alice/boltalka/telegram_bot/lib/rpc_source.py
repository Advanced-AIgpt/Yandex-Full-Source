from alice.boltalka.telegram_bot.lib.module import Module
import xmlrpc.client


class RpcSource(Module):
    class Options:
        addr = ''
        source_type = None

    def set_args(self, args):
        super().set_args(args)
        self.rpc = xmlrpc.client.ServerProxy(self.addr, allow_none=True)

    def get_candidates(self, args, context):
        self.set_args(args)
        return self.rpc.get_candidates(args, context)

    def get_options(self, args):
        self.set_args(args)
        return super().get_options(args) + self.rpc.get_options(args)

    def set_option(self, args, option, value):
        self.set_args(args)
        try:
            super().set_option(args, option, value)
        except KeyError:
            self.rpc.set_option(args, option, value)
