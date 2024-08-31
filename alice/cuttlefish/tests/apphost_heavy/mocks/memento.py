class Memento:
    backend_name = "MEMENTO_PROXY"

    @classmethod
    def auto(cls, name, request):
        return {"code": 200, "reason": "OK", "headers": [["Content-Type", "application/protobuf"]], "body": b"DEADBEEF"}
