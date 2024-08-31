# coding: utf-8


class VinsError(Exception):
    '''Базовые ошибки в VINS'''
    pass


class VinsAppLogicError(VinsError):
    '''Логическая ошибка на сервере'''
    pass


class VinsWebHookError(VinsAppLogicError):
    '''Логическая ошибка на сервере'''
    pass


class VinsBadRequestError(VinsError):
    '''Если запрос сформирован неправильно'''
    def __init__(self, msg):
        msg = "Mallformed request: %s" % msg
        super(VinsBadRequestError, self).__init__(msg)
