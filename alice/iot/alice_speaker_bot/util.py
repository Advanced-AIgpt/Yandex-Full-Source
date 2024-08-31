from requests import Session
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

from collections import defaultdict, OrderedDict


def build_menu(buttons, n_cols, header_buttons=None, footer_buttons=None):
    menu = [buttons[i:i + n_cols] for i in range(0, len(buttons), n_cols)]
    if header_buttons:
        menu.insert(0, [header_buttons])
    if footer_buttons:
        menu.append([footer_buttons])
    return menu


def group_by_room(devices):
    rooms = defaultdict(list)
    for d in devices:
        new_room = d.get("room_name")
        if new_room is not None:
            rooms[new_room].append(d)
        else:
            rooms['без комнаты'].append(d)

    sorted_rooms = OrderedDict()
    if 'без комнаты' in rooms:
        sorted_rooms['без комнаты'] = rooms.get('без комнаты')
        del rooms['без комнаты']

    rooms = OrderedDict(sorted(rooms.items()))
    for k, v in rooms.items():
        sorted_rooms[k] = v

    return sorted_rooms


def get_chat_id(update):
    return update.message.chat.id if update.message is not None else update.callback_query.message.chat.id


def retrying_session(max_retries=5):
    """
    Builds a :py:class:`Session` configured to retry requests on server-side errors at lease `max_retries` times.
    :param max_retries:
    :return:
    """
    session = Session()
    retries = Retry(total=max_retries,
                    status_forcelist=[500, 502, 503, 504],
                    raise_on_status=True,
                    method_whitelist=set())

    session.mount('http://', HTTPAdapter(max_retries=retries))
    session.mount('https://', HTTPAdapter(max_retries=retries))
    return session


class UnauthorizedError(Exception):
    def __init__(self, **kwargs):
        self.data = kwargs


class MyInternalError(Exception):
    def __init__(self, msg, **kwargs):
        self.msg = msg
        self.data = kwargs
