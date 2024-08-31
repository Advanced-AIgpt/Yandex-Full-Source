from .handlers import setup_handlers


def _jupyter_server_extension_paths():
    return [{"module": "jupytercloud_lab_vault"}]


def load_jupyter_server_extension(nb_server_app):
    setup_handlers(nb_server_app.web_app)
