# -*- coding: utf-8 -*-

import os
import requests
from getpass import getuser
from base64 import b64decode

SECRET_TEMPLATE = "http://localhost:8888/user/{username}/vault/secret/{secret}"

try:
    # NB: works only in Arcadia kernel
    from jupytercloud.nirvana.yav import JupyterYavClient
except ImportError:
    JupyterYavClient = None


def get_secret(version):
    # Works if and only if:
    # 1) We in Arcadia kernel
    # 2) There is JUPYTER_YAV_TOKEN environment variable
    # This environ fallback is supposed for usage inside Nirvana-operations
    if JupyterYavClient and JupyterYavClient.env_var in os.environ:
        token = os.getenv(JupyterYavClient.env_var)
        client = JupyterYavClient(token=token, ssh_key_fallback=False)

        # returns already unpacked and un-base64 data
        return client.get_secret(version=version)

    response = requests.get(SECRET_TEMPLATE.format(username=getuser(), secret=version))
    response.raise_for_status()

    response_data = response.json()

    result = {}
    for key in response_data:
        if response_data[key].get("encoding") == "base64":
            result[key] = b64decode(response_data[key]["value"])
        else:
            result[key] = response_data[key]["value"]

    return result
