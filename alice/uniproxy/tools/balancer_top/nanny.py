import urllib.request
import json


NANNY_URL = "http://nanny.yandex-team.ru"


def get_service_hostnames(service):
    url = f"{NANNY_URL}/v2/services/{service}/current_state/instances/partial/"
    resp = urllib.request.urlopen(url)
    instances = json.loads(resp.read().decode("utf-8"))
    return [i["container_hostname"] for i in instances["instancesPart"]]
