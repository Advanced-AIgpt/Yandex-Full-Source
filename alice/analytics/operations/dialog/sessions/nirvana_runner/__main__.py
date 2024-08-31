import os

from nile.api.v1 import cli

from alice.analytics.operations.dialog.sessions.lib_for_py2.make_dialog_sessions import make_sessions


@cli.statinfra_job(
    allow_optional_sources=True,
    options=[
        cli.Option('date'),
        cli.Option('sessions-root'),
        cli.Option('users-path'),
        cli.Option('devices-path'),
        cli.Option('expboxes'),
    ],
)
def cli_run(job, options):
    return make_sessions(job, os.path.join(options.sessions_root, options.date), options.expboxes,
                         options.users_path, options.devices_path, options.date)


if __name__ == '__main__':
    cli.run()
