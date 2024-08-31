from alice.uniproxy.library.backends_common.apphost import AppHostHttpClient, Request as AppHostRequest, ItemFormat
from alice.uniproxy.library.global_counter import GlobalCounter

from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
from alice.cuttlefish.library.protos.context_save_pb2 import TContextSaveRequest, TContextSaveResponse
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo, TRequestContext
from alice.megamind.protos.speechkit.directives_pb2 import TDirective as TSpeechkitDirective

from google.protobuf import json_format
from google.protobuf.struct_pb2 import Struct as google_protobuf_Struct


class AppHostedContextSave:
    def __init__(self, system, experiments):
        self.experiments = experiments
        self.system = system
        self._directives = []
        self.funcs = {}

    def add_directive(self, directive, func, *args):
        self._directives.append(directive)
        name = directive.get("name")
        self.funcs[name] = (func, args)

    @property
    def unexecuted_funcs(self):
        return self.funcs.values()

    async def execute(self, user_ticket, message_id):
        try:
            GlobalCounter.U2_COUNT_CONTEXT_SAVE_SUMM.increment()

            context_save_request = TContextSaveRequest()
            for directive in self._directives:
                name = directive.get("name")
                payload = directive.get("payload", {})
                proto_payload = json_format.ParseDict(payload, google_protobuf_Struct())
                context_save_request.Directives.add().MergeFrom(TSpeechkitDirective(
                    Name=name,
                    Payload=proto_payload,
                ))
                try:
                    getattr(GlobalCounter, ("u2_cls_count_" + name + "_summ").upper()).increment()
                except:
                    self.system.logger.log_directive({
                        "ForEvent": message_id,
                        "type": "ContextSaveStrange",
                        "Body": {"reason": "Unknown directive " + name},
                    })

            # only exps without UaaS (this is the only needed field)
            req_ctx = TRequestContext()
            for key, value in self.experiments.items():
                req_ctx.ExpFlags[key] = value

            request = AppHostRequest(
                path="context_save",
                items={
                    "context_load_response": TContextLoadResponse(
                        UserTicket=user_ticket,
                    ),
                    "request_context": req_ctx,
                    "session_context": TSessionContext(
                        UserInfo=TUserInfo(
                            Puid=self.system.puid,
                        ),
                        AppId=self.system.app_id,
                    ),
                    "context_save_request": context_save_request,
                },
            )

            ah_client = AppHostHttpClient()
            ah_resp = await ah_client.fetch(request, request_timeout=5)

            items = [it for it in ah_resp.get_items(item_type="context_save_response", proto_type=TContextSaveResponse, item_format=ItemFormat.PROTOBUF)]
            if not items:
                GlobalCounter.U2_FAILED_CONTEXT_SAVE_SUMM.increment()
                self.system.logger.log_directive({
                    "ForEvent": message_id,
                    "type": "ContextSaveFailed",
                    "Body": {"reason": "CONTEXT_SAVE didn't save 'context_save_response' protobuf"},
                })
            else:
                context_save_resp = items[0].data

                # clear successful funcs
                self.funcs = dict((k, v) for k, v in self.funcs.items() if k in context_save_resp.FailedDirectives)

                if context_save_resp.FailedDirectives:
                    self.system.logger.log_directive({
                        "ForEvent": message_id,
                        "type": "ContextSaveFailed",
                        "Body": {"reason": "CONTEXT_SAVE failed directives: " + ", ".join(context_save_resp.FailedDirectives)},
                    })
                    for name in context_save_resp.FailedDirectives:
                        try:
                            getattr(GlobalCounter, ("u2_cls_diff_" + name + "_summ").upper()).increment()
                        except:
                            self.system.logger.log_directive({
                                "ForEvent": message_id,
                                "type": "ContextSaveStrange",
                                "Body": {"reason": "Unknown directive " + name},
                            })
                else:
                    self.system.logger.log_directive({
                        "ForEvent": message_id,
                        "type": "ContextSaveSuccess",
                        "Body": {"reason": "CONTEXT_SAVE success!"},
                    })
        except Exception as exc:
            GlobalCounter.U2_FAILED_CONTEXT_SAVE_SUMM.increment()
            self.system.logger.log_directive({
                "ForEvent": message_id,
                "type": "ContextSaveFailed",
                "Body": {"reason": "CONTEXT_SAVE error: " + str(exc)},
            })
