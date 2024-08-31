# flake8: noqa
from collections import namedtuple

import requests

Panel = namedtuple("Panel", ["type", "title", "url", "rowspan", "colspan", "markdown"], defaults=["", "", "", 0, 0, ""])


class DashboardBuilder:
    def __init__(self, oauth_token):
        self.oauth_token = oauth_token

    @staticmethod
    def url_query(*pairs):
        if len(pairs) % 2 != 0:
            raise Exception("Invalid number of parameters to construct url query")
        ipairs = iter(pairs)
        return "?{query}".format(
            query="&".join(map(lambda pair: "{k}={v}".format(k=pair[0], v=pair[1]), zip(ipairs, ipairs))))

    @staticmethod
    def row_element(panel):
        return {
            "title": panel.title,
            "type": panel.type if panel.type else "IFRAME",
            "url": panel.url,
            "markdown": panel.markdown,
            "rowspan": panel.rowspan,
            "colspan": panel.colspan,
        }

    @staticmethod
    def row(panels):
        return {
            "panels": list(map(DashboardBuilder.row_element, panels))
        }

    @staticmethod
    def dashboard_from_rows(dashboard_id, project_id, name, parameters, rows, height_multiplier=1, version=0):
        return {
            "id": dashboard_id,
            "projectId": project_id,
            "name": name,
            "parameters": parameters,
            "heightMultiplier": height_multiplier,
            "rows": list(map(DashboardBuilder.row, rows)),
            "version": version,
        }

    def headers(self):
        return {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "Authorization": "OAuth {token}".format(token=self.oauth_token),
        }

    def create(self, project_id, dashboard_id, name, parameters, rows, height_multiplier=1):
        print("Creating dashboard {id} in project {prj}".format(id=dashboard_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/dashboards".format(prj=project_id)
        body = self.dashboard_from_rows(dashboard_id, project_id, name, parameters, rows, height_multiplier)
        response = requests.post(url, json=body, headers=self.headers())
        print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
        if response.status_code == 200:
            print("Created dashboard {id} in project {prj}".format(id=dashboard_id, prj=project_id))
        else:
            print("Failed to create dashboard {id} in project {prj}".format(id=dashboard_id, prj=project_id))

    def update(self, project_id, dashboard_id, name, parameters, rows, height_mult=1):
        print("Updating dashboard {id} in project {prj}".format(id=dashboard_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/dashboards/{id}".format(prj=project_id, id=dashboard_id)
        response = requests.get(url, headers=self.headers())
        if response.status_code == 404:
            self.create(project_id, dashboard_id, name, parameters, rows)
        elif response.status_code == 200:
            version = response.json().get("version", 0)
            body = self.dashboard_from_rows(dashboard_id, project_id, name, parameters, rows, height_mult, version)
            response = requests.put(url, json=body, headers=self.headers())
            print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
            if response.status_code == 200:
                print("Updated dashboard {id} in project {prj}".format(id=dashboard_id, prj=project_id))
            else:
                print("Failed to update dashboard {id} in project {prj}".format(id=dashboard_id, prj=project_id))
        else:
            print("Failed to update dashboard {id} in project {prj}".format(id=dashboard_id, prj=project_id))
