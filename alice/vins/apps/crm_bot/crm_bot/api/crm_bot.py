import logging

from vins_core.utils.datetime import timestamp_in_ms, utcnow

from personal_assistant.api.personal_assistant import PersonalAssistantAPI

logger = logging.getLogger(__name__)


class CrmBotAPI(PersonalAssistantAPI):
    def fake_submit_form(self, bass_result, req_info, form):
        return bass_result

    def _post(self, req_info, type_, data, balancer_type, **kwargs):
        market_reqid = req_info.additional_options.get('market_reqid', None)
        # speechkit api won't provide market_requid in additional options
        if market_reqid is None:
            market_reqid = str(timestamp_in_ms(utcnow())) + '/' + req_info.request_id

        if 'headers' not in kwargs:
            kwargs['headers'] = {b'X-Market-Req-ID': market_reqid}
        else:
            kwargs['headers'].update({b'X-Market-Req-ID': market_reqid})

        result = super(CrmBotAPI, self)._post(req_info, type_, data, balancer_type, **kwargs)
        return result
