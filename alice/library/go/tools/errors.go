package tools

import (
	"crypto/x509"
	"net/url"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func IsSSLCertificateError(err error) bool {
	var urlError *url.Error
	if xerrors.As(err, &urlError) {
		switch urlError.Err.(type) {
		case x509.CertificateInvalidError, x509.UnknownAuthorityError, x509.ConstraintViolationError, x509.HostnameError, x509.SystemRootsError:
			return true
		}
	}
	return false
}
