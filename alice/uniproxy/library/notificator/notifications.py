import tornado
import time

from alice.protos.api.notificator.api_pb2 import TDeletePersonalCards
from alice.uniproxy.library.notificator.common_handler import NotificatorCommonRequestHandler
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.personal_cards import PersonalCardsHelper


# ====================================================================================================================
class DeletePersonalCardsHandler(NotificatorCommonRequestHandler):
    unistat_handler_name = 'delete_personal_cards'

    @tornado.gen.coroutine
    def post(self):
        start = time.monotonic()
        try:
            request = self.get_proto_request(TDeletePersonalCards)
            if request is None:
                GlobalCounter.DELETE_PERSONAL_CARDS_FAIL_SUMM.increment()
                self.INFO("can't parse request")
                self.set_status(404)
                self.finish()
                return

            helper = PersonalCardsHelper(request.Puid, self._rt_log)
            ok = yield helper.remove_cards_by_tag(request.Directive.Tag)
            if not ok:
                GlobalCounter.DELETE_PERSONAL_CARDS_FAIL_SUMM.increment()
                self.INFO("get personal cards failed")
                self.set_status(404)
                self.finish()
                return

            GlobalCounter.DELETE_PERSONAL_CARDS_OK_SUMM.increment()
            self.set_status(200)
            self.finish()

        except Exception as exc:
            GlobalCounter.DELETE_PERSONAL_CARDS_FAIL_SUMM.increment()
            self.INFO(str(exc))
            self.set_status(500)
            self.finish('exception occured')

        finally:
            GlobalTimings.store('delete_personal_cards', time.monotonic() - start)
