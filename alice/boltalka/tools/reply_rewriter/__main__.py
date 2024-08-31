import argparse
import sys
import codecs
import yt.wrapper as yt
from alice.boltalka.tools.reply_rewriter.substitute_rewritten_replies import RewrittenReplacer
from alice.boltalka.tools.reply_rewriter.substitute_replies import SubstituteReplacer
from alice.boltalka.tools.reply_rewriter.sed_replacer import SedReplacer
from alice.boltalka.tools.reply_rewriter.pipeline_replacer import PipelineReplacer
from alice.boltalka.tools.reply_rewriter.respect_replacer import RespectReplacer
from alice.boltalka.tools.reply_rewriter.capitalizer import Capitalizer

yt.config.set_proxy("hahn.yt.yandex.net")


class PipelineManager(object):
    def __init__(self):
        self.parser = argparse.ArgumentParser(add_help=True)
        self.parser.add_argument('--src', default='')
        self.parser.add_argument('--dst', default='')
        self.parser.add_argument('--local', action='store_true')
        self.subparsers = self.parser.add_subparsers(
            title="pipeline", dest="pipeline")
        self.pipelines = {}

    def register_pipeline(self, name, pipeline):
        self.pipelines[name] = pipeline
        subparser = self.subparsers.add_parser(name)
        for stage in pipeline:
            stage.register_args(subparser)

    def run_yt(self, pipeline, args):
        assert args.src and args.dst
        row_count = yt.get(args.src + '/@row_count')
        rows_per_job = 10000
        job_count = min((row_count + rows_per_job - 1) // rows_per_job, 1000)
        mapper = PipelineReplacer(pipeline, args)
        yt.run_map(
            mapper,
            args.src,
            args.dst,
            spec={'job_count': job_count},
            **mapper.get_yt_extra_args())

    def run_local(self, pipeline, args):
        sys.stdin = codecs.getreader('utf-8')(sys.stdin)
        sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
        sys.stderr = codecs.getwriter('utf-8')(sys.stderr)
        pipeline = PipelineReplacer(pipeline, args)
        pipeline.start(local=True)

        for line in sys.stdin:
            reply = pipeline.process(line.strip())
            print reply

    def run(self, args):
        assert args.pipeline in self.pipelines
        pipeline = self.pipelines[args.pipeline]
        if args.local:
            self.run_local(pipeline, args)
        else:
            self.run_yt(pipeline, args)

    def cli_parse_args(self):
        return self.parser.parse_args()

    def cli_run(self):
        self.run(self.cli_parse_args())


if __name__ == '__main__':
    manager = PipelineManager()
    manager.register_pipeline(
        "rewrite", [SubstituteReplacer, SedReplacer, RewrittenReplacer])
    manager.register_pipeline("add_more_respect", [RespectReplacer])
    manager.register_pipeline("capitalize", [Capitalizer])
    manager.cli_run()
