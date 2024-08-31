from yql.client.operation import YqlOperationShareIdRequest


def get_yql_share_url(operation_id):
    share_request = YqlOperationShareIdRequest(operation_id)
    share_request.run()
    return 'https://yql.yandex-team.ru/Operations/{id}'.format(id=share_request.json)
