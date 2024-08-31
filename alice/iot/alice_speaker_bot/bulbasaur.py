# -*- coding: utf-8 -*-
import logging

from urllib.parse import urljoin

import requests

from util import UnauthorizedError, MyInternalError


logger = logging.getLogger(__name__)


class BulbasaurClient:
    def __init__(self, url):
        self.url = url

    def user_info(self, token):
        headers = {'Authorization': "OAuth {token}".format(token=token)}
        response = requests.get(url=urljoin(self.url, '/api/v1.0/user/info'),
                                headers=headers)
        if response.status_code == 401:
            raise UnauthorizedError()
        elif response.status_code != 200:
            logger.info("bad response: {}".format(response.status_code))
            logger.info(response.text)
            raise MyInternalError(response.status_code)

        return UserInfo(response.json())

    def scenario_run(self, token, scenario_id):
        headers = {'Authorization': "OAuth {token}".format(token=token)}
        response = requests.post(url=urljoin(self.url, '/api/v1.0/scenarios/{scenario_id}/actions'.format(scenario_id=scenario_id)),
                                 headers=headers)
        if response.status_code == 401:
            raise UnauthorizedError()
        elif response.status_code != 200:
            logger.info("bad response: {}".format(response.status_code))
            logger.info(response.text)
            raise MyInternalError(response.status_code)

        return None


class UserInfo:
    def __init__(self, user_info):
        self.user_info = user_info

    def _get_rooms_map(self):
        rooms_map = dict()
        for room in self.user_info.get('rooms', list()):
            rooms_map[room['id']] = room['name']
        return rooms_map

    def get_speakers(self):
        devices = list()
        rooms_map = self._get_rooms_map()
        for d in self.user_info.get('devices', list()):
            if d['type'].startswith("devices.types.smart_speaker"):
                d = dict(
                    name=d['name'],
                    room_name=rooms_map.get(d['room']),
                    id=d['external_id'].split(".")[0]
                )
                devices.append(d)
        return devices

    def get_rooms_with_lights(self):
        rooms, rooms_set = list(), set()
        rooms_map = self._get_rooms_map()
        for d in self.user_info.get('devices', list()):
            if d['type'].startswith("devices.types.light"):
                r = dict(
                    name=rooms_map.get(d['room']),
                    id=d['room']
                )
                if r['id'] in rooms_set:
                    continue
                rooms_set.add(r['id'])
                rooms.append(r)
        return rooms

    def get_scenarios(self):
        scenarios = list()
        for s in self.user_info.get('scenarios', list()):
            if s.get('is_active'):
                scenarios.append(s)
        return scenarios

    def get_rooms(self):
        pass    # not implemented yet
