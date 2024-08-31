import yaml
import os
import json


MAPPER_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
INFO_YAML_PATH = os.path.join(MAPPER_DIR, "app_info.yaml")
CLIENTS_JSON_PATH = os.path.join(MAPPER_DIR, "utils/clients_FILTERED.json")


with open(INFO_YAML_PATH, "r") as f:
    infos = yaml.load(f)

clients = []
with open(CLIENTS_JSON_PATH, "r") as f:
    for l in f.readlines():
        if l:
            clients.append(json.loads(l))


def find_info(app_id):
    for i in infos:
        if i["AppId"] == app_id:
            return i
    return None


def print_info(info):
    def _yaml_field(key):
        val = info[key]
        if not val:
            val = "''"
        return f"{key}: {val}"

    res = (
        f"\n-   {_yaml_field('AppId')}"
        f"\n    {_yaml_field('Surface')}"
        f"\n    {_yaml_field('Platform')}"
        f"\n    {_yaml_field('Type')}"
        f"\n    {_yaml_field('Vendor')}"
    )

    if "AuthTokens" in info:
        res += "\n    AuthTokens:"
        for x in info["AuthTokens"]:
            res += f"\n    -    {x}"

    return res + "\n"

other_tokens = set()
for x in clients:
    app_id = x["app_id"]

    if app_id is None:
        print(f"DROPPED: {json.dumps(x)}")
        other_tokens.add(x["auth_token"])
        continue

    info = find_info(app_id.lower())
    if info is None:
        print(f"DROPPED: {json.dumps(x)}")
        other_tokens.add(x["auth_token"])
        continue

    tokens = info.setdefault("AuthTokens", [])
    if x["auth_token"] not in tokens:
        tokens.append(x["auth_token"])

with open("./app_info_NEW.yaml", "w") as f:
    f.writelines(print_info(x) for x in infos)

with open("./other_tokens.txt", "w") as f:
    f.writelines(f"{x}\n" for x in other_tokens)
