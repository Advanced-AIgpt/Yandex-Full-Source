# coding: utf-8

import uuid
import logging

from vins_core.utils.datetime import timestamp_in_ms, utcnow

logger = logging.getLogger(__name__)


class MarketRequestIdMiddleware(object):
    def __init__(self, reqid_header):
        self.header_name = reqid_header

    def process_request(self, req, resp):
        if 'webim' not in req.relative_uri and 'ocrm' not in req.relative_uri:
            return

        # assign req_id here so we can log it along with market-reqid
        market_reqid_candidate = req.get_header(self.header_name)
        try:
            req.context['reqid'] = market_reqid_candidate.split('/')[1]
            req.context['market_reqid'] = market_reqid_candidate
            logger.debug("Using MarketRequestId from header: {}".format(req.context['market_reqid']))
        except (IndexError, AttributeError):
            uuid_str = uuid.uuid4().hex
            req.context['market_reqid'] = str(timestamp_in_ms(utcnow())) + '/' + uuid_str
            req.context['reqid'] = uuid_str
            logger.debug("Assigned new MarketRequestId: {}".format(req.context['market_reqid']))

    def process_resource(self, req, resp, resource, params):
        pass

    def process_response(self, req, resp, resource, req_succeeded):
        pass
