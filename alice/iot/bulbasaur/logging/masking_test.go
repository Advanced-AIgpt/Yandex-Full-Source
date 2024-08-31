package logging

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestBlackbox(t *testing.T) {
	URL := "https://blackbox.yandex.net/blackbox?format=json&get_user_ticket=yes&method=sessionid&sessionid=secret_cookie_value&sslsessionid=secret_cookie_value"

	if rule := GetRule(URL); rule != nil {
		URL = rule.ProcessURL(URL)
	}

	assert.NotContains(t, URL, "secret_cookie")
}
