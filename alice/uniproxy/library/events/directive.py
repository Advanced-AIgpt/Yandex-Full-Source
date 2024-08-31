from uuid import uuid4
import time


class Directive:
    def __init__(
        self,
        namespace,
        name,
        payload,
        event_id=None,
        async_message_id=None,
        stream_id=None,
        need_ack=False,
        ack_data=None,
        ack_time=None,
    ):
        self.namespace = namespace
        self.name = name
        self.payload = payload
        self.event_id = event_id
        self.async_message_id = async_message_id
        self.stream_id = stream_id
        self.need_ack = need_ack
        self.ack_data = ack_data
        self.ack_time = ack_time

    def create_message(self, system):
        if system:
            self.message_id = system.next_message_id()
        else:
            self.message_id = str(uuid4())
        header = {
            "namespace": self.namespace,
            "name": self.name,
            "messageId": self.message_id,
        }
        if self.event_id is not None:
            if self.namespace + self.name == "ASRResult":
                header["eventId"] = self.event_id
            header["refMessageId"] = self.event_id

        if self.stream_id is not None:
            header["streamId"] = self.stream_id

        if self.async_message_id:
            header["transferId"] = self.async_message_id

        if self.need_ack:
            if self.ack_time is None:
                self.ack_time = int(time.time() * 1000)
            header['ack'] = self.ack_time
            system.acks[header['messageId']] = {
                'name': self.namespace.lower() + '.' + self.name.lower(),
                'data': self.ack_data,
            }

        return {
            "directive": {
                "header": header,
                "payload": self.payload
            }
        }
