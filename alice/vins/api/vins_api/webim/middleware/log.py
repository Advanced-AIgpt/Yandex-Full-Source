# coding: utf-8

import logging
import time

from alice.vins.api_helper.resources import dump_request_for_log, parse_json_request
from vins_core.utils.config import get_setting
from vins_core.utils.datetime import utcnow

logger = logging.getLogger(__name__)


def to_tskv(data):
    return "tskv\ttskv_format=trace-log\t" + '\t'.join(['{}={}'.format(k, v) for k, v in data.iteritems()])


class IOLogMiddleware(object):
    def process_request(self, req, resp):
        if 'webim' not in req.relative_uri and 'ocrm' not in req.relative_uri:
            return

        # save timestamp
        req.context['processing_begin'] = time.time()

        req.context['body'] = parse_json_request(req)
        logger.info(dump_request_for_log(
            {
                'request': {
                    'uri': req.relative_uri,
                    'from': req.access_route,
                    'body': req.context['body']
                },
                'market_reqid': req.context['market_reqid'],
                'reqid': req.context['reqid']
            }
        ))

    def process_resource(self, req, resp, resource, params):
        pass

    def process_response(self, req, resp, resource, req_succeeded):
        if 'webim' not in req.relative_uri:
            return
        result = resp.context.get('result')
        if result is not None:
            result_json = result.to_json()
        else:
            result_json = None
        logger.info(dump_request_for_log(
            {
                'request': {
                    'uri': req.relative_uri,
                    'from': req.access_route,
                    'body': req.context['body']
                },
                'result': result_json,
                'response': {
                    'body': resp.body,
                    'status_code': resp.status
                },
                'reqid': req.context['reqid'],
                'success': req_succeeded
            }
        ))
        if get_setting('TSUM_TRACE_LOG', default='') != '':
            # save a record for TSUM trace
            record = {
                'date': utcnow().strftime("%Y-%m-%dT%H:%M:%S.%f%z"),
                'request_id': req.context['market_reqid'],
                'source_module': '',
                'source_host': req.access_route[0],
                'target_module': 'lilucrmchat',
                'target_host': req.netloc,
                'request_method': req.path,
                'http_code': str(resp.status)[0:3],
                'retry_num': '',
                'duration_ms': int((time.time() - req.context['processing_begin']) * 1000),
                'error_code': '',
                'protocol': req.forwarded_scheme,
                'http_method': req.method,
                'type': 'IN',
                'query_params': req.relative_uri,
                'response_size_bytes': len(resp.body)
            }
            logging.getLogger('TSUM_trace').info(to_tskv(record))
