import os
import json


MAPPER_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
CLIENTS_JSON_PATH = os.path.join(MAPPER_DIR, "utils/clients.json")
CLIENTS_FILTERED_JSON_PATH = os.path.join(MAPPER_DIR, "utils/clients_FILTERED.json")
ORPHANED_TOKENS_LIST_PATH = os.path.join(MAPPER_DIR, "utils/orphaned_tokens.txt")
SETTINGS_JSON_PATH = os.path.join(MAPPER_DIR, "../../../uniproxy/library/settings/rtc_production.json")


with open(SETTINGS_JSON_PATH, "r") as f:
    whitelist = {x for x in json.load(f)["apikeys"]["whitelist"]}

clients = []
with open(CLIENTS_JSON_PATH, "r") as f:
    for l in f.readlines():
        if not l:
            continue
        client = json.loads(l)
        if client["auth_token"] in whitelist:
            clients.append(client)

with open(CLIENTS_FILTERED_JSON_PATH, "w") as f:
    for client in clients:
        f.write(json.dumps(client) + "\n")

with open(ORPHANED_TOKENS_LIST_PATH, "w") as f:
    for token in whitelist:
        found = False
        for client in clients:
            if client["auth_token"] == token:
                found = True
                break
        if not found:
            f.write(token + "\n")
