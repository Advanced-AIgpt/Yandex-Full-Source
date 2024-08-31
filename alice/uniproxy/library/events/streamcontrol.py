from alice.uniproxy.library.events.event_exception import EventException


class StreamControl(object):
    class ActionType(object):
        CLOSE = 0
        CHUNK = 1
        SPOTTER_END = 2
        NEXT_CHUNK = 3

    def __init__(self, obj, is_client):
        try:
            self.message_id = obj["messageId"]
            self.reason = obj["reason"]
            self.action = obj["action"]
            self.size = obj.get("size", None)
            self.stream_id = obj["streamId"]
        except KeyError as exc:
            if is_client:
                raise EventException("StreamControl without %s" % (exc,))
            else:
                raise Exception("StreamControl from server is invalid")

    def create_message(self, system):
        return {
            "streamcontrol":
            {
                "messageId": system.next_message_id(),
                "action": self.action,
                "reason": self.reason,
                "streamId": self.stream_id
            }
        }

    @staticmethod
    def okClose(system, stream_id):
        return StreamControl(
            {
                "messageId": system.next_message_id(),
                "reason": 0,
                "action": StreamControl.ActionType.CLOSE,
                "streamId": stream_id
            },
            False
        )

    @staticmethod
    def try_parse(message):
        streamcontrol_json = message.get("streamcontrol")
        if streamcontrol_json:
            return StreamControl(streamcontrol_json, True)
        return None
