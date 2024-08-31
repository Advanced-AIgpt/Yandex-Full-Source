# flake8: noqa
import attr

import requests


@attr.s
class RGBA(object):
    r = attr.ib(default=0)
    g = attr.ib(default=0)
    b = attr.ib(default=0)
    a = attr.ib(default=None)

    def expression_color(self):
        if self.a is None:
            return "rgb({r}, {g}, {b})".format(r=self.r, g=self.g, b=self.b)
        return "rgba({r}, {g}, {b}, {a})".format(r=self.r, g=self.g, b=self.b, a=self.a)


@attr.s
class Expression(object):
    title = attr.ib()
    expression = attr.ib()
    color = attr.ib(default=None)
    area = attr.ib(default=True)
    stack = attr.ib(default=None)


class GraphicBuilder:
    def __init__(self, oauth_token):
        self.oauth_token = oauth_token

    @staticmethod
    def graphic_element(expression):
        element = {
            "title": expression.title,
            "type": "EXPRESSION",
            "expression": expression.expression,
            "area": expression.area,
        }
        if expression.stack is not None:
            element["stack"] = "On" if expression.stack else "Off"
        if expression.color is not None:
            element["color"] = expression.color.expression_color()
        return element

    @staticmethod
    def graphic_from_expressions(graphic_id, project_id, name, parameters, expressions, extra_data=None, version=0):
        if extra_data is None:
            extra_data = {}
        return {
            "id": graphic_id,
            "projectId": project_id,
            "name": name,
            "parameters": parameters,
            "elements": list(map(GraphicBuilder.graphic_element, expressions)),
            "min": extra_data.get("min", ""),
            "max": extra_data.get("max", ""),
            "scale": extra_data.get("scale", "NATURAL"),
            "interpolate": extra_data.get("interpolate", "LINEAR"),
            "normalize": extra_data.get("normalize", False),
            "version": version,
            "graphMode": extra_data.get("graphMode", "GRAPH"),
            "aggr": extra_data.get("aggr", "AVG")
        }

    def headers(self):
        return {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "Authorization": "OAuth {token}".format(token=self.oauth_token),
        }

    def create(self, project_id, graphic_id, name, parameters, expressions, extra_data=None):
        if extra_data is None:
            extra_data = {}
        print("Creating graphic {id} in project {prj}".format(id=graphic_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/graphs".format(prj=project_id)
        body = self.graphic_from_expressions(graphic_id, project_id, name, parameters, expressions, extra_data)
        response = requests.post(url, json=body, headers=self.headers())
        print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
        if response.status_code == 200:
            print("Created graphic {id} in project {prj}".format(id=graphic_id, prj=project_id))
        else:
            print("Failed to create graphic {id} in project {prj}".format(id=graphic_id, prj=project_id))

    def update(self, project_id, graphic_id, name, parameters, expressions, extra_data=None):
        if extra_data is None:
            extra_data = {}
        print("Updating graphic {id} in project {prj}".format(id=graphic_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/graphs/{id}".format(prj=project_id, id=graphic_id)
        response = requests.get(url, headers=self.headers())
        if response.status_code == 404:
            self.create(project_id, graphic_id, name, parameters, expressions, extra_data)
        elif response.status_code == 200:
            version = response.json().get("version", 0)
            body = self.graphic_from_expressions(graphic_id, project_id, name, parameters, expressions, extra_data,
                                                 version)
            response = requests.put(url, json=body, headers=self.headers())
            print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
            if response.status_code == 200:
                print("Updated graphic {id} in project {prj}".format(id=graphic_id, prj=project_id))
            else:
                print("Failed to update graphic {id} in project {prj}".format(id=graphic_id, prj=project_id))
        else:
            print("Failed to update graphic {id} in project {prj}".format(id=graphic_id, prj=project_id))
