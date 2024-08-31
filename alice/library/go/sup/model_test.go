package sup

import (
	"net/url"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestLinkWrappers(t *testing.T) {
	deviceListLink := "https://yandex.ru/quasar/iot"
	wrappedDeviceList := SearchAppLinkWrapper(QuasarLinkWrapper(deviceListLink))

	// generated on https://mobile-board.yandex-team.ru/deeplink_generator_mobsearch
	expectedWrappedLink := "ya-search-app-open://?uri=yellowskin%3A%2F%2F%3Furl%3Dhttps%253A%252F%252Fyandex.ru%252Fquasar%252Fiot"
	assert.Equal(t, expectedWrappedLink, wrappedDeviceList)
}

func TestQueryEscape(t *testing.T) {
	link := "https://yandex.ru/quasar/?tab=scenarios"
	expected := "https%3A%2F%2Fyandex.ru%2Fquasar%2F%3Ftab%3Dscenarios"
	assert.Equal(t, expected, url.QueryEscape(link))
}
