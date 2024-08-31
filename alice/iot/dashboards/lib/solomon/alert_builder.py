# flake8: noqa
import attr
import requests

@attr.s
class Alert(object):
    program = attr.ib()
    annotations = attr.ib()
    labels = attr.ib(default=[])


class AlertBuilder:
    def __init__(self, oauth_token):
        self.oauth_token = oauth_token

    @staticmethod
    def alert_body(alert_id, project_id, name, alert, channels, period_millis, version=0):
        return {
            "id": alert_id,
            "projectId": project_id,
            "name": name,
            "groupByLabels": alert.labels,
            "notificationChannels": channels,
            "type": {
                "expression": {
                    "program": alert.program.replace('                ', ''),
                    "checkExpression": ""
                }
            },
            "annotations": alert.annotations,
            "periodMillis": period_millis,
            "version": version,
        }

    def headers(self):
        return {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "Authorization": "OAuth {token}".format(token=self.oauth_token),
        }

    def create(self, project_id, alert_id, name, alert, channels, period_millis):
        print("Creating alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/alerts".format(prj=project_id)
        body = self.alert_body(alert_id, project_id, name, alert, channels, period_millis)
        response = requests.post(url, json=body, headers=self.headers())
        print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
        if response.status_code == 200:
            print("Created alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        else:
            print("Failed to create alert {id} in project {prj}".format(id=alert_id, prj=project_id))

    def update(self, project_id, alert_id, name, alert, channels, period_millis):
        print("Updating alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/alerts/{id}".format(prj=project_id, id=alert_id)
        response = requests.get(url, headers=self.headers())
        if response.status_code == 404:
            self.create(project_id, alert_id, name, alert, channels, period_millis)
        elif response.status_code == 200:
            version = response.json().get("version", 0)
            body = self.alert_body(alert_id, project_id, name, alert, channels, period_millis, version)
            response = requests.put(url, json=body, headers=self.headers())
            print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
            if response.status_code == 200:
                print("Updated alert {id} in project {prj}".format(id=alert_id, prj=project_id))
            else:
                print("Failed to update alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        else:
            print("Failed to update alert {id} in project {prj}".format(id=alert_id, prj=project_id))


class AlertBuilderV2:
    """Supports more fields. Some deprecated fields are changed to the actual ones"""
    def __init__(self, oauth_token):
        self.oauth_token = oauth_token

    @staticmethod
    def alert_body(alert_id, project_id, name, alert, channels, window_secs, delay_secs, description='',  version=0):
        return {
            "id": alert_id,
            "projectId": project_id,
            "name": name,
            "description": description,
            "groupByLabels": alert.labels,
            "channels": channels,
            "type": {
                "expression": {
                    "program": alert.program.replace('                ', ''),
                    "checkExpression": ""
                }
            },
            "annotations": alert.annotations,
            "windowSecs": window_secs,
            "delaySecs": delay_secs,
            "version": version,
        }

    def headers(self):
        return {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "Authorization": "OAuth {token}".format(token=self.oauth_token),
        }

    def create(self, alert_id, project_id, name, alert, channels, window_secs=15, delay_secs=0, description=''):
        print("Creating alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/alerts".format(prj=project_id)
        body = self.alert_body(alert_id, project_id, name, alert, channels, window_secs, delay_secs, description)
        response = requests.post(url, json=body, headers=self.headers())
        print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
        if response.status_code == 200:
            print("Created alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        else:
            print("Failed to create alert {id} in project {prj}".format(id=alert_id, prj=project_id))

    def update(self, alert_id, project_id, name, alert, channels, window_secs=15, delay_secs=0, description=''):
        print("Updating alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        url = "https://solomon.yandex.net/api/v2/projects/{prj}/alerts/{id}".format(prj=project_id, id=alert_id)
        response = requests.get(url, headers=self.headers())
        if response.status_code == 404:
            self.create(alert_id, project_id, name, alert, channels, window_secs, delay_secs, description)
        elif response.status_code == 200:
            version = response.json().get("version", 0)
            body = self.alert_body(alert_id, project_id, name, alert, channels, window_secs, delay_secs, description, version)
            response = requests.put(url, json=body, headers=self.headers())
            print("Response code is {code}, body: {text}".format(code=response.status_code, text=response.json()))
            if response.status_code == 200:
                print("Updated alert {id} in project {prj}".format(id=alert_id, prj=project_id))
            else:
                print("Failed to update alert {id} in project {prj}".format(id=alert_id, prj=project_id))
        else:
            print("Failed to update alert {id} in project {prj}".format(id=alert_id, prj=project_id))

