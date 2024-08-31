package userip

import (
	"net"
	"net/http"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// GetUserIPFromRequest returns user real IP. First it looks ip in balancer headers.
// If IP is not found in headers then returns ip from request.RemoteAddr
func GetUserIPFromRequest(r *http.Request) (string, error) {
	if xffy := r.Header.Get("X-Forwarded-For-Y"); xffy != "" {
		return strings.Split(xffy, ",")[0], nil
	} else if xff := r.Header.Get("X-Forwarded-For"); xff != "" {
		return strings.Split(xff, ",")[0], nil
	} else if xri := r.Header.Get("X-Real-Ip"); xri != "" {
		return xri, nil
	} else {
		ip, _, err := net.SplitHostPort(r.RemoteAddr)
		if err != nil {
			return "", xerrors.Errorf("failed to split remoteAddr using net.SplitHostPort: %q. Reason: %w", r.RemoteAddr, err)
		}
		return ip, nil
	}
}
