import asyncio
import json
from copy import deepcopy
from datetime import date
import re
from typing import Iterable, Optional
import pkgutil

import click
import aiohttp
from tqdm import tqdm
import yaml

from statface_client import ProductionStatfaceClient as StatfaceClient, DAILY_SCALE
from jupytercloud.tools.lib import cloud, environment, utils

YANDEX = 962
ANALYTIC_RE = r"(?:анали|analy)"
ANALYTIC_EXCLUSION_RE = r"(?:аналитическ)"
STAFF_API = "https://staff-api.yandex-team.ru/v3/"

logger = None
session: Optional[aiohttp.ClientSession] = None


def debug_json(obj) -> str:
    return json.dumps(obj, ensure_ascii=False, sort_keys=True, indent=2)


async def staff_get_pages(relative_uri, params, *, description=None):
    async def append_data(data, response, progress):
        response_data = await response.json()
        update = response_data.get("result", [])
        data.extend(update)

        progress.total = response_data["total"]
        progress.update(len(update))

        return response_data["links"].get("next", "")

    data = []
    next_url = None

    with tqdm(desc=description) as progress:
        while next_url != "":
            request = session.get(
                url=STAFF_API + relative_uri if next_url is None else next_url,
                params=params if next_url is None else None,
            )
            async with request as response:
                next_url = await append_data(data, response, progress=progress)

    return data


async def staff_get_departments():
    raw_departments = await staff_get_pages(
        "groups",
        {"_sort": "id", "_fields": "id,name,ancestors.id,ancestors.name", "type": "department", "_limit": 1000},
        description="Departments from Staff",
    )

    departments = {}
    for dep in raw_departments:
        id_ = dep["id"]
        name = dep["name"]

        departments[id_] = {
            "id": id_,
            "name": name,
            "ancestors": {
                "id": [x["id"] for x in dep.get("ancestors", [])] + [id_],
                "name": [x["name"] for x in dep.get("ancestors", [])] + [name],
            },
        }

    return departments


async def staff_get_users():
    raw_users = await staff_get_pages(
        "persons",
        {
            "_sort": "id",
            "_fields": "id,login,department_group.ancestors.id,department_group.id,official.position.ru",
            "official.is_dismissed": "false",
            "official.is_robot": "false",
            "_limit": 1000,
        },
        description="Yandex employees from Staff",
    )

    users = {}
    for user in raw_users:
        users[user["login"]] = {
            "login": user["login"],
            "id": user["id"],
            "position": user["official"]["position"]["ru"],
            "department_ancestors": {
                "id": [x["id"] for x in user["department_group"]["ancestors"]] + [user["department_group"]["id"]],
            },
        }

    return users


def mark_jupyter_users(people, jupyter_users):
    result = deepcopy(people)
    for person in result:
        result[person]["jupyter_user"] = person in jupyter_users
    return result


def check_analytic_string(position: str) -> bool:
    return (
        re.search(ANALYTIC_RE, position, re.IGNORECASE) is not None
        and re.search(ANALYTIC_EXCLUSION_RE, position, re.IGNORECASE) is None
    )


def mark_analysts(people, departments, department_blacklist):
    def check_analytics_departments(person, departments):
        result = False
        for department_id in person["department_ancestors"]["id"]:
            if department_id in department_blacklist or department_id not in departments:
                return False

            if check_analytic_string(departments[department_id]["name"]):
                result = True
        return result

    result = deepcopy(people)
    for login in result:
        result[login]["analyst_position"] = check_analytic_string(result[login]["position"])
        result[login]["analyst_department"] = check_analytics_departments(result[login], departments)
        result[login]["analyst"] = result[login]["analyst_position"] or result[login]["analyst_department"]

    return result


def count_analysts(people, departments):
    for dep in departments:
        departments[dep]["total_people"] = 0
        departments[dep]["jupyter_users"] = 0

        # lower bound, underestimates
        departments[dep]["analysts_position"] = 0
        departments[dep]["jupyter_analysts_position"] = 0

        # upper bound, overestimates
        departments[dep]["analysts"] = 0
        departments[dep]["jupyter_analysts"] = 0

    for login in people:
        for dep in people[login]["department_ancestors"]["id"]:
            if dep not in departments:
                continue

            departments[dep]["total_people"] += 1
            departments[dep]["jupyter_users"] += people[login]["jupyter_user"]

            departments[dep]["analysts_position"] += people[login]["analyst_position"]
            departments[dep]["jupyter_analysts_position"] += (
                people[login]["jupyter_user"] and people[login]["analyst_position"]
            )

            departments[dep]["analysts"] += people[login]["analyst"]
            departments[dep]["jupyter_analysts"] += people[login]["jupyter_user"] and people[login]["analyst"]

    return departments


def tree_field(array: Iterable) -> str:
    return "\t" + "\t".join(map(str, array)) + "\t"


def form_stat_data(departments, fielddate):
    result = []

    for department in departments.values():
        if department["ancestors"]["id"][0] != YANDEX:
            continue

        result.append(
            {
                "fielddate": fielddate,
                "department": tree_field(department["ancestors"]["name"]),
                "people": department["total_people"],
                "jupyter_users": department["jupyter_users"],
                "analysts_position": department["analysts_position"],
                "jupyter_analysts_position": department["jupyter_analysts_position"],
                "analysts": department["analysts"],
                "jupyter_analysts": department["jupyter_analysts"],
            }
        )

    return result


def upload_stat_data(data, client):
    report = client.get_report("Statface/JupyterCloud/Coverage")
    return report.upload_data(scale=DAILY_SCALE, data=data, replace_mask=["fielddate"])


async def calculate_coverage(yav_token, verbose, fielddate):
    global logger
    logger = utils.setup_logging(__name__, verbose)

    if not fielddate:
        logger.debug("fielddate is None, setting it to today")
        fielddate = date.today().isoformat()
    logger.info("fielddate=%s", fielddate)

    with environment.environment("production", yav_oauth_token=yav_token):
        jupyter_cloud = cloud.JupyterCloud()
        staff_oauth = environment.get("staff_oauth_token")

        headers = {
            "Authorization": f"OAuth {staff_oauth}",
            "User-Agent": "jupytercloud-tools-calc_company_coverage/1.0",
        }
        global session
        session = aiohttp.ClientSession(headers=headers)

        people = await staff_get_users()
        logger.info("Got %s users", len(people))
        logger.debug("Users: %s", debug_json(people))

        jupyter_users = set(jupyter_cloud.get_active_users())
        logger.info("Got %s Jupyter users", len(jupyter_users))
        logger.debug("Jupyter users: %s", debug_json(list(jupyter_users)))

        people = mark_jupyter_users(people, jupyter_users)
        logger.info("Marked %s Jupyter users", sum((people[x]["jupyter_user"] for x in people)))

        departments = await staff_get_departments()
        logger.info("Got %s departments", len(departments))
        logger.debug("Departments: %s", debug_json(departments))

        department_blacklist = yaml.safe_load(pkgutil.get_data(__package__, "blacklist.yaml"))
        logger.info("Loaded department blacklist: %s", department_blacklist)

        people = mark_analysts(people, departments, department_blacklist)
        logger.info("Marked %s Analysts", sum((people[x]["analyst"] for x in people)))
        logger.debug("Users: %s", debug_json(people))

        departments = count_analysts(people, departments)
        logger.info("Counted analysts in departments")
        logger.debug("Departments: %s", debug_json(departments))

        data = form_stat_data(departments, fielddate)
        logger.info("Formed Statface data, %s rows", len(data))
        logger.debug("Statface data: %s", debug_json(data))

        statface_client = StatfaceClient(oauth_token=environment.get("statface_token"))
        response = upload_stat_data(data, statface_client)
        logger.info("Uploaded data to Statface, response: %s", response)

        await session.close()


@click.command()
@click.option("--yav_token", envvar=["YAV_TOKEN", "CUSTOM_TOKEN_1"])  # custom token is for Nirvana operation
@click.option("--verbose", "-v", default=0)
@click.option("--fielddate", default=None)
def main(yav_token, verbose, fielddate):
    asyncio.run(calculate_coverage(yav_token, verbose, fielddate))
