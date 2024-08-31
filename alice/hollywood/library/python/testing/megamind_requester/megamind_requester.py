import copy
import json
import logging
import requests
import uuid

logger = logging.getLogger(__name__)


def request_megamind(servers, sk_req, srcrwr_params, additional_params=[]):
    code, sk_run_resp_str = _request_megamind(
        servers,
        sk_req['header']['request_id'],
        json.dumps(sk_req),
        sk_req_to_str_for_logging(sk_req),
        srcrwr_params,
        additional_params=additional_params,
    )
    assert code == 200, f'Request (run) to megamind returned status_code={code}, content={sk_run_resp_str}'

    sk_resp = json.loads(sk_run_resp_str)
    directives = sk_resp['response']['directives']
    if directives and directives[0]['name'] == 'defer_apply' and directives[0]['type'] == 'uniproxy_action':
        session_from_defer_apply = directives[0]['payload']['session']
        sk_req_apply = _make_sk_req_apply_from(sk_req, session_from_defer_apply)
        code, sk_apply_resp_str = _request_megamind(
            servers,
            sk_req['header']['request_id'],
            json.dumps(sk_req_apply),
            sk_req_to_str_for_logging(sk_req_apply),
            srcrwr_params,
            is_apply=True,
            additional_params=additional_params,
        )
        assert code == 200, f'Request (apply/commit) to megamind returned status_code={code}, ' \
            f'content={sk_apply_resp_str}'
        return sk_run_resp_str, sk_apply_resp_str

    return (sk_run_resp_str, )


def find_session(sk_resp):
    sessions = list(sk_resp['sessions'].values())
    session = sessions[0] if len(sessions) > 0 else None
    return session


def make_request_id_base(app_preset, test_name):
    hex_word_marker = 'dabbadoo00'
    request_id_base = str(uuid.uuid5(uuid.NAMESPACE_OID, f'{app_preset}.{test_name}'))
    return request_id_base[:-len(hex_word_marker)] + hex_word_marker


def make_request_id(request_id_base, run_request_index):
    index_suffix = f'{run_request_index:02d}'
    return request_id_base[:-len(index_suffix)] + index_suffix


def sk_req_to_str_for_logging(sk_req):
    sk_req2 = copy.deepcopy(sk_req)
    additional_options = sk_req2.get('request', {}).get('additional_options', {})
    additional_options['oauth_token'] = '******'
    if 'guest_user_options' in additional_options:
        additional_options['guest_user_options']['oauth_token'] = '**OBFUSCATED**'
    return json.dumps(sk_req2, indent=4, ensure_ascii=False)


def _request_megamind(servers, sk_request_id, sk_request_json, sk_request_for_logging, srcrwr_params, is_apply=False, additional_params=[]):
    url = servers.apphost.apply_url if is_apply else servers.apphost.url
    url += f'?srcrwr={servers.megamind.srcrwr}&srcrwr={servers.scenario_runtime.srcrwr}'
    url += '&timeout=10000&waitall=da&dump_source_requests=1&dump_source_responses=1'

    for key, val in srcrwr_params.items():
        url += f'&srcrwr={key}:{val}'

    logger.info(f'additional_params: {additional_params}')
    for val in additional_params:
        url += f'&{val}'

    session = requests.Session()
    session.headers.update({
        'Content-Type': 'application/json',
        'X-Yandex-Internal-Request': '1',
        'X-Yandex-Req-Id': sk_request_id,
    })
    logger.info(f'Requesting megamind, Url: {url}, Headers: {session.headers}, Data: {sk_request_for_logging}')
    response = session.post(url, data=sk_request_json)
    logger.info(f'Megamind response from Url: {url} returned Code: {response.status_code}, Content: {response.text}')
    return response.status_code, response.text


def _make_sk_req_apply_from(sk_req_run, session):
    sk_req = copy.deepcopy(sk_req_run)
    sk_req['session'] = session
    logger.info(f'Generated speech kit request (apply) is: {sk_req_to_str_for_logging(sk_req)}')
    return sk_req
