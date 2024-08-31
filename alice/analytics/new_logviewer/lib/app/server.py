import flask
import yaml
import time
import os
import json
import statface_client
import logging
import logging.config

from threading import Thread
from datetime import timedelta

from yt.wrapper import YtClient

from alice.analytics.new_logviewer.lib.app.metrics import LogviewerMetrics
from alice.analytics.new_logviewer.lib.app.logviewer_config import LogviewerConfig
from alice.analytics.new_logviewer.lib.app.forms import SearchForm

from alice.analytics.new_logviewer.lib.search.logviewer_request import LogviewerRequest
from alice.analytics.new_logviewer.lib.search.logviewer_executor import LogviewerExecutor

CONTENT_TYPE_SPACK = "application/x-solomon-spack"
CONTENT_TYPE_JSON = "application/json"

APP_CHOICES = [
    "",
    "browser_alpha",
    "browser_beta",
    "browser_prod",
    "stroka",
    "yabro_beta",
    "yabro_prod",
    "launcher",
    "small_smart_speakers",
    "navigator",
    "search_app_beta",
    "search_app_prod",
    "elariwatch",
    "auto",
    "quasar",
    "centaur",
    "alice_app",
    "yandexmaps_dev",
    "yandexmaps_prod",
    "taximeter",
    "tv",
    "music_app_prod",
    "auto_old",
    "other"
]

skills: list[str] = list()
skill_to_key: dict[str, str] = dict()

logger = logging.getLogger("logviewer.server")
logviewer_metrics = LogviewerMetrics()


def download_skills():
    global skills, skill_to_key
    client = statface_client.ProductionStatfaceClient(oauth_token=os.environ["STATFACE_TOKEN"])
    key_to_skill = json.loads(client.get_stat_dict("external_skills_pa").download())
    skill_to_key = {key_to_skill[key]: key for key in key_to_skill}
    skill_to_key[str()] = str()
    skills = sorted(skill_to_key.keys())


def update_skills():
    while True:
        requests_start = time.time()
        download_skills()
        logviewer_metrics.skill_response_time.collect((time.time() - requests_start) * 1000)
        period = int(timedelta(days=1).total_seconds())
        time.sleep(period)


def run_server(config_yaml):
    app = flask.Flask("logviewer")
    app.secret_key = os.environ["FLASK_SECRET_KEY"]
    app.config['WTF_CSRF_ENABLED'] = False

    config_dict = yaml.safe_load(config_yaml)
    config = LogviewerConfig(config_dict)

    metrics_config = config.to_logviewer_metrics_config()
    logviewer_metrics.init_from_config(metrics_config)

    download_skills()
    Thread(target=update_skills).start()

    yt_client = YtClient(proxy=config.cluster, token=os.environ["YT_TOKEN"])

    logging.config.dictConfig(config.logging_config)

    @app.route("/", methods=["GET"])
    def search():
        form = SearchForm(APP_CHOICES, skills, csrf_enabled=False, **flask.request.args)
        if form.validate():
            logviewer_metrics.search_requests_per_second.inc()
            requests_start = time.time()

            request = LogviewerRequest(**flask.request.args)
            request.skill_id = skill_to_key[request.skill_id]

            try:
                executor = LogviewerExecutor(request, config.to_logviewer_executor_config(), yt_client)
                response = executor.execute()
                logviewer_metrics.search_response_time.collect((time.time() - requests_start) * 1000)
            except Exception as exception:
                logger.error(exception)
                logviewer_metrics.search_errors_per_second.inc()
                return flask.render_template(config.view, form=form, show_mode=False, data=None, error=str(exception))

            return flask.render_template(config.view, form=form, show_mode=True, data=response, error="")
        return flask.render_template(config.view, form=form, show_mode=False, data=None, error="")

    @app.route("/metrics")
    def metrics():
        return logviewer_metrics.to_response()

    app.run(host=config.host, port=config.port)
