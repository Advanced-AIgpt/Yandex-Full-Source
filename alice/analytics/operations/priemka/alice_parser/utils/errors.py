# coding: utf-8

from builtins import object


class Ue2eResultError(object):
    UNIPROXY_ERROR = 'UNIPROXY_ERROR'
    EMPTY_VINS_RESPONSE = 'EMPTY_VINS_RESPONSE'
    NONEMPTY_SIDESPEECH_RESPONSE = 'NONEMPTY_SIDESPEECH_RESPONSE'
    EMPTY_SIDESPEECH_RESPONSE = 'EMPTY_SIDESPEECH_RESPONSE'
    RENDER_ERROR = 'RENDER_ERROR'


ALL_PREDIFINED_RESULTS = [var for var in vars(Ue2eResultError) if not var.startswith("__")]
ILL_URL_RESULT = 'ill_url'

# неответы, при такой ошибки нельзя оценивать запрос и считать по нему метрику
FULL_UNASWERS = (
    Ue2eResultError.UNIPROXY_ERROR,
    Ue2eResultError.RENDER_ERROR,
    ILL_URL_RESULT,
)


def get_preliminary_downloader_result(basket_text, req_id, vins_response, generic_scenario):
    """
    Определяет тип ошибки прокачки для одного запроса:
        * UNIPROXY_ERROR — на случай неответа
        * EMPTY_VINS_RESPONSE - пустой ответ сервера
        * EMPTY_SIDESPEECH_RESPONSE — ответ спец. классификатора side_speech, пустой ответ Алисы
        * NONEMPTY_SIDESPEECH_RESPONSE - любой ответ сервера там, где должен быть пустой ответ
    :param Optional[str] basket_text:
    :param Optional[str] req_id:
    :param Optional[dict] vins_response:
    :param Optional[dict] generic_scenario:
    :return Optional[str]:
    """
    if req_id is None:
        return Ue2eResultError.UNIPROXY_ERROR
    if vins_response is None:
        return Ue2eResultError.EMPTY_VINS_RESPONSE
    if generic_scenario == 'side_speech':
        return Ue2eResultError.EMPTY_SIDESPEECH_RESPONSE
    if basket_text is not None and basket_text == '':
        return Ue2eResultError.NONEMPTY_SIDESPEECH_RESPONSE
    return None


def get_most_session_unanswer(records_list):
    """
    Возвращает тип наибольшей ошибки в любом из запросов внутри сессии или None, если ошибок не было
    Учитываются только "важные" ошибки — если они произошли в контекстных запросах, то мы считаем,
        что сессию целиком нельзя считать валидной и оценивать в Толоке.
    Здесь не учитываются RENDER_ERROR и ошибки (false positive) сценария side_speech
    :param list[dict] records_list:
    :return Optional[str]:
    """
    downloader_results = [r.get('result') for r in records_list]
    for error in (
        Ue2eResultError.UNIPROXY_ERROR,
        Ue2eResultError.EMPTY_VINS_RESPONSE,
    ):
        if error in downloader_results:
            return error
    return None


def get_render_error_status(last_record):
    """
    Возвращает ошибку RENDER_ERROR, если у оцениваемого запроса в сессии нет скриншота, иначе None
    :param dict last_record:
    :return Optional[str]:
    """
    if last_record.get('screenshot_absent') is True:
        # режим парсинга без скриншотов
        return None

    if (
        'result' in last_record and last_record.get('result') is None and
        'action' in last_record and last_record.get('action', {}).get('url') is None
    ):
        return Ue2eResultError.RENDER_ERROR
