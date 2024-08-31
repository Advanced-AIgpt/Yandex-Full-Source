
class MessengerError(Exception):
    def __init__(self, code, response, raw=False):
        super().__init__()
        self.code = code
        self.response = response
        self.raw = raw


# ====================================================================================================================
class AuthenticationError(Exception):
    def __init__(self):
        super().__init__()


# ====================================================================================================================
class DeliveryError(Exception):
    def __init__(self, code, message, name='DeliveryError'):
        super(DeliveryError, self).__init__('{}: {}'.format(name, message))
        self.code = code
        self.message = message


# --------------------------------------------------------------------------------------------------------------------
class InvalidMessageError(DeliveryError):
    def __init__(self, reason):
        super(InvalidMessageError, self).__init__(
            400,
            'invalid message, reason: {}'.format(reason),
            name='InvalidMessageError'
        )


# --------------------------------------------------------------------------------------------------------------------
class NoServiceTicketError(DeliveryError):
    def __init__(self):
        super().__init__(
            401,
            'request MUST contain X-Ya-Service-Ticket http header',
            name=self.__class__.__name__,

        )


# --------------------------------------------------------------------------------------------------------------------
class InvalidVersionError(DeliveryError):
    def __init__(self, version):
        super(InvalidVersionError, self).__init__(
            400,
            'invalid message version - got {} expected {}'.format(version, 2),
            name='InvalidVersionError'
        )


# --------------------------------------------------------------------------------------------------------------------
class InvalidChecksumError(DeliveryError):
    def __init__(self, checksum, expected_checksum):
        super(InvalidChecksumError, self).__init__(
            400,
            '400 BadRequest',
            name='InvalidChecksumError'
        )
        self._checksum = checksum
        self._expected = expected_checksum


# --------------------------------------------------------------------------------------------------------------------
class NoLocationError(DeliveryError):
    def __init__(self, guid):
        super(NoLocationError, self).__init__(
            200,
            'no location for guids={}'.format(guid),
            name='NoLocationError'
        )
