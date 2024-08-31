from nile.api.v1 import cli

from alice.analytics.operations.dialog.sessions.lib_for_py2.make_video_squeeze import prepare_table


@cli.statinfra_job(
    allow_optional_sources=True,
    options=[
        cli.Option('date', required=True),
        cli.Option('sessions-root', required=True),
        cli.Option('output-root', required=True),
    ],
)
def cli_run(job, options):
    return prepare_table(job, options.date, options.sessions_root, options.output_root)


if __name__ == '__main__':
    cli.run()
