import json
from alice.cuttlefish.library.python.test_utils import deepupdate, get_at


class Megamind:
    backend_name = "CUTTLEFISH_MEGAMIND_URL"

    @classmethod
    def _standard_response_for_text_input(cls):
        return {
            "header": {
                "dialog_id": None,
                "sequence_number": 1,
            },
            "response": {
                "card": {
                    "buttons": [{"directives": [], "title": "some button", "type": "action"}],
                    "text": "some response text",
                    "type": "text_with_button",
                },
                "cards": [
                    {
                        "buttons": [{"directives": [], "title": "another title", "type": "action"}],
                        "text": "another response text",
                        "type": "text_with_button",
                    }
                ],
                "directives": [],
                "directives_execution_policy": "BeforeSpeech",
                "experiments": {},
                "quality_storage": {},
                "templates": {},
            },
            "sessions": {"": "e30="},
            "voice_response": {
                "directives": [
                    {
                        "name": "update_datasync",
                        "payload": {
                            "key": "/v1/personality/profile/alisa/kv/proactivity_history",
                            "listening_is_possible": True,
                            "method": "PUT",
                            "value": """{"RequestCount":"1","LastStorageUpdateTime":"1616833286"}""",
                        },
                        "type": "uniproxy_action",
                    }
                ],
                "should_listen": False,
            },
        }

    @classmethod
    def auto(cls, name, request):
        response = cls._standard_response_for_text_input()

        msg = json.loads(request["body"].decode("utf-8"))
        if get_at(msg, "request", "voice_session", default=False):
            deepupdate(
                response, {"voice_response": {"output_speech": {"type": "simple", "text": "text to be said aloud"}}}
            )

        if get_at(msg, "request", "event", "text") == "APPLY":
            pass  # TODo

        return {
            "code": 200,
            "reason": "OK",
            "headers": [["Content-Type", "application/json"]],
            "body": json.dumps(response),
        }
