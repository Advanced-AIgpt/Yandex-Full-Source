import uuid


class Mds:
    backend_name = "VOICE__MDS_STORE_HTTP"

    @classmethod
    def auto(cls, name, request):
        file_id = uuid.uuid4()
        return {
            "code": 200,
            "reason": "OK",
            "body": (
                '<?xml version="1.0" encoding="utf-8"?>'
                f'<post obj="namespace.filename" id="{file_id}" groups="3" size="100" key="221/filename-{file_id}">'
                '</post>'
            ),
        }
