package xos

import (
	"os"

	"a.yandex-team.ru/library/go/yandex/yplite"
)

// GetHostname returns current hostname. If YP api is available then return pod name in YP
// otherwise returns os.Hostname()
func GetHostname() string {
	if yplite.IsAPIAvailable() {
		spec, ypErr := yplite.FetchPodSpec()
		if ypErr == nil {
			return spec.DNS.PersistentFqdn
		}
	} else {
		hostname, err := os.Hostname()
		if err == nil {
			return hostname
		}
	}
	return "unavailable"
}
