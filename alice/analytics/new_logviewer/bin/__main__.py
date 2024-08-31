import click


from alice.analytics.new_logviewer.lib.app.server import run_server


@click.command()
@click.argument('config', required=True, type=click.File())
def main(config):
    run_server(config)


if __name__ == "__main__":
    main()
