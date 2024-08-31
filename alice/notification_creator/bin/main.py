import alice.notification_creator.lib.app as app

import gunicorn.app.base

import argparse
import json
import logging


class GunicornApplication(gunicorn.app.base.BaseApplication):
    def __init__(self, flask_app, options=None):
        self.options = options or {}
        self.flask_app = flask_app
        super(GunicornApplication, self).__init__()
        gunicorn_logger = logging.getLogger('gunicorn.error')
        self.flask_app.logger.handlers = gunicorn_logger.handlers
        self.flask_app.logger.setLevel(gunicorn_logger.level)

    def load_config(self):
        config = {key: value for key, value in self.options.items()
                  if key in self.cfg.settings and value is not None}
        for key, value in config.items():
            self.cfg.set(key.lower(), value)

    def load(self):
        return self.flask_app


def build_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", default="config.json")
    return parser


def main():
    args = build_parser().parse_args()
    with open(args.config, 'r') as f:
        config = json.load(f)

    server_app = app.create_app(config['app'], __name__)
    server_app.logger = logging.getLogger('gunicorn.error')

    server = GunicornApplication(
        flask_app=server_app,
        options=config["server"]
    )
    server.run()


if __name__ == "__main__":
    main()
