from alice.cachalot.api.protos import cachalot_pb2
import logging


class Cachalot:
    logger = logging.getLogger("mock.cachalot")

    @classmethod
    async def process(cls, name, request):
        mm_req = request.get_item("request", cachalot_pb2.TMegamindSessionRequest)
        if mm_req.HasField("LoadRequest"):
            request.add_item(
                item_type="response",
                item=cachalot_pb2.TResponse(
                    Status=cachalot_pb2.EResponseStatus.NO_CONTENT, StatusMessage="Sorry, there is no content"
                ),
            )
        else:
            request.add_item(
                item_type="response",
                item=cachalot_pb2.TResponse(Status=cachalot_pb2.EResponseStatus.CREATED, StatusMessage="Created"),
            )
