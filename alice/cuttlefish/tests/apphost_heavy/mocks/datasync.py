class Datasync:
    backend_name = "VOICE__DATASYNC"

    @classmethod
    def auto(cls, name, request):
        return {"code": 200, "reason": "OK"}
