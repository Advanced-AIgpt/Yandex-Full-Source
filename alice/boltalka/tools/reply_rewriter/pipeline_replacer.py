from alice.boltalka.tools.reply_rewriter.base_replacer import BaseReplacer


class PipelineReplacer(BaseReplacer):
    def __init__(self, replacers, args):
        self.pipeline = [stage(args) for stage in replacers]

    def start(self, local=False):
        for stage in self.pipeline:
            stage.start(local)

    def process(self, reply):
        for stage in self.pipeline:
            reply = stage.process(reply)
        return reply

    def process_row(self, row):
        for stage in self.pipeline:
            row = stage.process_row(row)
        return row

    def get_yt_extra_args(self):
        args = {}
        for stage in self.pipeline:
            current_args = stage.get_yt_extra_args()
            for k, v in current_args.items():
                if k == "local_files":
                    if k not in args:
                        args[k] = v
                    else:
                        args[k] += v
                else:
                    raise KeyError("Unknown extra argument")
        return args
