import copy
import json
import os
import requests
import time

from random import randint

import yatest.common

from alice.personal_cards.integration_tests.lib.tvmknife import TVMKnife

import ydb


def gen_uid():
    return abs(int(time.time() + randint(0, 2**64)) % int(10**62))


class TestHandlersResponse(object):
    """
        Test personal cards handlers
    """

    deprecated_routes = [
        "/addCards",
        "/personalCards",
        "/updateCards",
        "/userContext",
        "/loadCards",
        "/updateCardsData",
        "/userInfo",
        "/getStoriesInfo",
        "/addStoriesWatch",
    ]

    dump_path = yatest.common.get_param("dump_file_path", "request_response_dump.log")

    tvm_aliases = {
        "/cards": "/getPushCards",
        "/dismiss": "/removePushCard",
        "/addPushCards": "/addPushCard"
    }

    def _get_tvm_alias(self, route):
        return self.tvm_aliases[route]

    def _get_add_push_card_req(self, uid, card_id=None, tag=None):
        return {
            "card": {
                "card": {
                    "card_id": card_id or "station_billing_test",
                    "tag": tag or "video",
                    "data": {
                        "button_url": "https://ya.ru",
                        "min_price": "100500",
                        "text": "Hello world!",
                    },
                     "date_from": int(time.time()),
                     "date_to": int(time.time()) + 86400,  # cur time + seconds in day,
                     "type": "yandex.station_film"
                },
                "sent_date_time": {
                    "$date": int(time.time() * 1000)
                }
            },
            "uid": str(uid)
        }

    def _get_dismiss_push_card_req(self, uid, card_id=None):
        return {
            "auth": {
                "uid": str(uid)
            },
            "card_id": card_id or "station_billing_test",
        }

    def _get_cards_req(self, uid, client_id, uid_cast=str):
        return {
            "auth": {
                "uid": uid_cast(uid)
            },
            "client_id": client_id
        }

    def _get_push_cards(self, personal_cards_url, uid, client_id="search_app", alias_f=lambda x: x, drop_uid=False):
        def get(uid_cast=str):
            req = self._get_cards_req(uid, client_id, uid_cast=uid_cast)
            if drop_uid:
                assert req["auth"]["uid"]
                del req["auth"]["uid"]
                assert "uid" not in req["auth"]

            req["bigb"] = {"smth": "value"}
            req["some_data"] = "hello"
            res = self._perform_request(personal_cards_url, alias_f("/cards"), req, int(uid)).json()
            assert "bigb" not in res
            assert "some_data" in res
            assert res["some_data"] == "hello"

            return res

        uid_as_int = get(uid_cast=int)
        uid_as_str = get(uid_cast=str)
        if "uid" in uid_as_int.get("auth", {}):
            uid_as_int["auth"]["uid"] = str(uid_as_int["auth"]["uid"])
        assert uid_as_str == uid_as_int

        return uid_as_str.get("blocks", None)

    def _get_table_prefix(self, table_path):
        return os.path.join(os.getenv('YDB_DATABASE') + table_path)

    def _create_session(self):
        driver = ydb.Driver(ydb.DriverConfig(os.getenv('YDB_ENDPOINT'), os.getenv('YDB_DATABASE')))
        driver.wait()
        session = ydb.retry_operation_sync(lambda: driver.table_client.session().create())
        return session

    def _get_push_cards_from_ydb_table(self, uid, table_path):
        session = self._create_session()
        table_prefix = self._get_table_prefix(table_path)
        query = f'PRAGMA TablePathPrefix("{table_prefix}");\nSELECT * FROM push_cards WHERE uid = "uid/{uid}";'
        return session.transaction().execute(query)[0].rows

    def _describe_table(self, table_path):
        session = self._create_session()
        table_prefix = self._get_table_prefix(table_path)
        return session.describe_table(os.path.join(table_prefix, "push_cards"))

    def _perform_request(self, personal_cards_url, path, payload, uid=123):
        headers = {}
        if path in self.tvm_aliases.values():
            tvmknife = TVMKnife()
            headers = {
                "X-Ya-Service-Ticket": tvmknife.service_ticket(),
                "X-Ya-User-Ticket": tvmknife.user_ticket(uid),
            }

        if not hasattr(self, 'session'):
            self.session = requests.Session()
            self.session.mount("http://", requests.adapters.HTTPAdapter(max_retries=5))

        response = self.session.post(personal_cards_url + path, json=payload, headers=headers)

        response.raise_for_status()

        rsp_to_canonize = copy.deepcopy(response.json())
        payload_to_canonize = copy.deepcopy(payload)
        if path == "/addPushCards" or path == "/addPushCard":
            payload_to_canonize["card"]["card"]["date_from"] = None
            payload_to_canonize["card"]["card"]["date_to"] = None
            payload_to_canonize["card"]["sent_date_time"]["$date"] = None
            payload_to_canonize["uid"] = None
        elif path == "/cards" or path == "/getPushCards":
            payload_to_canonize["auth"]["uid"] = None
            rsp_to_canonize["auth"]["uid"] = None
            blocks = rsp_to_canonize.get("blocks", [])
            if blocks:
                for i in range(len(blocks)):
                    rsp_to_canonize["blocks"][i]["date_from"] = None
                    rsp_to_canonize["blocks"][i]["date_to"] = None
        elif path == "/dismiss" or path == "/removePushCard":
            payload_to_canonize["auth"]["uid"] = None

        with open(self.dump_path, 'a') as f:
            log_line = f'Path={path}, Request={json.dumps(payload_to_canonize, sort_keys=True)}, Code={response.status_code}, Response={json.dumps(rsp_to_canonize, sort_keys=True)}'
            print(log_line, file=f)
        return response

    def test_deprecated_routes(self, personal_cards_url, cfg):
        for route in self.deprecated_routes:
            response = self._perform_request(personal_cards_url, route, {})
            assert response.status_code == requests.codes.ok

    def _check_add_dismiss(self, personal_cards_url, alias_f):
        uid_1 = gen_uid()
        assert not self._get_push_cards(personal_cards_url, uid_1, alias_f=alias_f)

        req = self._get_add_push_card_req(uid_1, card_id="card_id_1", tag="video")
        self._perform_request(personal_cards_url, alias_f("/addPushCards"), req, int(uid_1))

        cards = self._get_push_cards(personal_cards_url, uid_1, alias_f=alias_f)
        assert len(cards) == 1
        card = cards[0]
        assert card["card_id"] == req["card"]["card"]["card_id"]
        assert card["tag"] == req["card"]["card"]["tag"]
        assert card["data"] == req["card"]["card"]["data"]
        assert card["type"] == req["card"]["card"]["type"]
        assert card["date_from"] == req["card"]["card"]["date_from"]
        assert card["date_to"] == req["card"]["card"]["date_to"]

        #  Change something in existing push card and check
        req["card"]["card"]["type"] = "another_type"
        self._perform_request(personal_cards_url, alias_f("/addPushCards"), req, uid_1)
        cards = self._get_push_cards(personal_cards_url, uid_1, alias_f=alias_f)
        assert len(cards) == 1
        assert cards[0]["type"] == req["card"]["card"]["type"]

        def convert(cards):
            res = {}
            for card in cards:
                res[card["card_id"]] = card

            return res

        # Add one more cards for uid_1
        req = self._get_add_push_card_req(uid_1, card_id="card_id_2", tag="stream")
        self._perform_request(personal_cards_url, alias_f("/addPushCards"), req, int(uid_1))
        cards = self._get_push_cards(personal_cards_url, uid_1, alias_f=alias_f)
        assert len(cards) == 2
        cards = convert(cards)
        assert "card_id_1" in cards
        assert "card_id_2" in cards
        assert cards["card_id_1"]["tag"] == "video"
        assert cards["card_id_2"]["tag"] == "stream"
        card = cards["card_id_2"]
        assert card["card_id"] == req["card"]["card"]["card_id"]
        assert card["tag"] == req["card"]["card"]["tag"]
        assert card["data"] == req["card"]["card"]["data"]
        assert card["type"] == req["card"]["card"]["type"]
        assert card["date_from"] == req["card"]["card"]["date_from"]
        assert card["date_to"] == req["card"]["card"]["date_to"]

        #  Add second card, remove first and check table contents
        uid_2 = gen_uid()
        req = self._get_add_push_card_req(uid_2)
        self._perform_request(personal_cards_url, alias_f("/addPushCards"), req, uid_2)
        assert len(self._get_push_cards(personal_cards_url, uid_1, alias_f=alias_f)) == 2
        assert self._get_push_cards(personal_cards_url, uid_2, alias_f=alias_f)

        req = self._get_dismiss_push_card_req(uid_1, card_id="card_id_1")
        self._perform_request(personal_cards_url, alias_f("/dismiss"), req, uid_1)
        cards = self._get_push_cards(personal_cards_url, uid_1, alias_f=alias_f)
        assert len(cards) == 1
        assert cards[0]["card_id"] == "card_id_2"
        req = self._get_dismiss_push_card_req(uid_1, card_id="card_id_2")
        self._perform_request(personal_cards_url, alias_f("/dismiss"), req, uid_1)
        assert not self._get_push_cards(personal_cards_url, uid_1, alias_f=alias_f)
        assert len(self._get_push_cards(personal_cards_url, uid_2, alias_f=alias_f)) == 1

    def test_add_dismiss(self, personal_cards_url, cfg):
        return self._check_add_dismiss(personal_cards_url, lambda x: x)

    def test_add_dismiss_tvm(self, personal_cards_url, cfg):
        return self._check_add_dismiss(personal_cards_url, lambda x: self._get_tvm_alias(x))

    def test_tvm_uid_parse(self, personal_cards_url, cfg):
        uid_1 = gen_uid()

        # Add with /addPushCard (TVM)
        req = self._get_add_push_card_req(uid_1)
        assert req["uid"] == str(uid_1)
        del req["uid"]
        assert "uid" not in req
        self._perform_request(personal_cards_url, "/addPushCard", req, uid_1)

        # Get from /getPushCards (TVM)
        card = self._get_push_cards(personal_cards_url, uid_1, drop_uid=True, alias_f=lambda x: self._get_tvm_alias(x))[0]
        assert card
        assert card["card_id"] == req["card"]["card"]["card_id"]
        # Get from /cards
        card = self._get_push_cards(personal_cards_url, uid_1)[0]
        assert card
        assert card["card_id"] == req["card"]["card"]["card_id"]

        # Remove with /removePushCard (TVM)
        req = self._get_dismiss_push_card_req(uid_1)
        assert req["auth"]["uid"] == str(uid_1)
        del req["auth"]["uid"]
        assert "uid" not in req["auth"]
        self._perform_request(personal_cards_url, "/removePushCard", req, uid_1)
        # Get with /cards
        assert not self._get_push_cards(personal_cards_url, uid_1)
        # Get with /getPushCards
        assert not self._get_push_cards(personal_cards_url, uid_1, drop_uid=True, alias_f=lambda x: self._get_tvm_alias(x))

        # Add with /addPushCards
        req = self._get_add_push_card_req(uid_1)
        self._perform_request(personal_cards_url, "/addPushCards", req, uid_1)

        # Get from /getPushCards (TVM)
        card = self._get_push_cards(personal_cards_url, uid_1, drop_uid=True, alias_f=lambda x: self._get_tvm_alias(x))[0]
        assert card
        assert card["card_id"] == req["card"]["card"]["card_id"]
        # Get from /cards
        card = self._get_push_cards(personal_cards_url, uid_1)[0]
        assert card
        assert card["card_id"] == req["card"]["card"]["card_id"]

        # Remove with /dismiss
        req = self._get_dismiss_push_card_req(uid_1)
        self._perform_request(personal_cards_url, "/dismiss", req, uid_1)
        # Get with /cards
        assert not self._get_push_cards(personal_cards_url, uid_1)
        # Get with /getPushCards
        assert not self._get_push_cards(personal_cards_url, uid_1, drop_uid=True, alias_f=lambda x: self._get_tvm_alias(x))

    def test_ydb_remove_obsolete_push_cards(self, personal_cards_url, cfg):
        uid = gen_uid()
        req = self._get_add_push_card_req(uid)
        date_to = int(time.time())
        req["card"]["card"]["date_to"] = date_to
        self._perform_request(personal_cards_url, "/addPushCards", req)

        cards = self._get_push_cards_from_ydb_table(uid, cfg["YDBClient"]["Path"])
        assert cards
        sec_in_week = 604800
        assert cards[0]["expire_at"] == (date_to + sec_in_week) * 10**6

    def test_unknown_client_id(self, personal_cards_url, cfg):
        uid = gen_uid()
        req = self._get_add_push_card_req(uid)
        self._perform_request(personal_cards_url, "/addPushCards", req)

        assert self._get_push_cards(personal_cards_url, uid, "search_app")
        assert self._get_push_cards(personal_cards_url, uid, "some_unknown_client_id") is None

    def test_time_restrictions(self, personal_cards_url, cfg):
        uid = gen_uid()
        req = self._get_add_push_card_req(uid)
        sec_in_day = 86400
        now = int(time.time())

        # date_from > now
        req["card"]["card"]["date_from"] = now + sec_in_day
        req["card"]["card"]["date_to"] = req["card"]["card"]["date_from"] + sec_in_day * 2
        self._perform_request(personal_cards_url, "/addPushCards", req)
        assert not self._get_push_cards(personal_cards_url, uid)

        # date_to < now
        req["card"]["card"]["date_from"] = now - sec_in_day
        req["card"]["card"]["date_to"] = req["card"]["card"]["date_from"] + sec_in_day // 2
        self._perform_request(personal_cards_url, "/addPushCards", req)
        assert not self._get_push_cards(personal_cards_url, uid)

        # date_from < now < date_to
        req["card"]["card"]["date_from"] = now - sec_in_day
        req["card"]["card"]["date_to"] = now + sec_in_day
        self._perform_request(personal_cards_url, "/addPushCards", req)
        assert self._get_push_cards(personal_cards_url, uid)

    def test_read_replicas_settings(self, personal_cards_url, cfg):
        read_replicas_settings = self._describe_table(cfg["YDBClient"]["Path"]).read_replicas_settings
        assert read_replicas_settings
        assert read_replicas_settings.per_az_read_replicas_count == 2
        assert read_replicas_settings.any_az_read_replicas_count == 0

    def test_dismiss_all_cards_at_once(self, personal_cards_url, cfg):
        uid_1 = gen_uid()
        req = self._get_add_push_card_req(uid_1, card_id="card_id_1")
        self._perform_request(personal_cards_url, "/addPushCard", req, int(uid_1))
        assert len(self._get_push_cards(personal_cards_url, uid_1)) == 1

        uid = gen_uid()
        assert not self._get_push_cards(personal_cards_url, uid)

        req = self._get_add_push_card_req(uid, card_id="card_id_1")
        self._perform_request(personal_cards_url, "/addPushCard", req, int(uid))

        req = self._get_add_push_card_req(uid, card_id="card_id_2")
        self._perform_request(personal_cards_url, "/addPushCard", req, int(uid))

        assert len(self._get_push_cards(personal_cards_url, uid)) == 2

        req = self._get_dismiss_push_card_req(uid, "*")
        self._perform_request(personal_cards_url, "/removePushCard", req, int(uid))

        assert not self._get_push_cards(personal_cards_url, uid)
        assert len(self._get_push_cards(personal_cards_url, uid_1)) == 1
