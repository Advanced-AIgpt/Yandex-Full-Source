package tools

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

type testCase struct {
	base     string
	parts    []string
	expected string
	id       string
}

func TestURLJoin(t *testing.T) {
	cases := []testCase{
		{base: "http://base.com", parts: []string{"child"}, expected: "http://base.com/child", id: "1"},
		{base: "http://base.com/", parts: []string{"child"}, expected: "http://base.com/child", id: "2"},
		{base: "http://base.com/", parts: []string{"/child"}, expected: "http://base.com/child", id: "3"},
		{base: "http://base.com", parts: []string{"child/"}, expected: "http://base.com/child/", id: "4"},
		{base: "http://base.com", parts: []string{"child/second"}, expected: "http://base.com/child/second", id: "5"},
		{base: "http://base.com", parts: []string{"child?with=args"}, expected: "http://base.com/child%3Fwith=args", id: "6"},
		{base: "http://base.com", parts: []string{"child", "second"}, expected: "http://base.com/child/second", id: "7"},
		{base: "http://base.com/", parts: []string{"child", "second/"}, expected: "http://base.com/child/second/", id: "8"},
		{base: "http://base.com/", parts: []string{"child", "/second"}, expected: "http://base.com/child/second", id: "9"},
		{base: "http://base.com/", parts: []string{"child", "/second/"}, expected: "http://base.com/child/second/", id: "10"},
		{base: "http://base.com", parts: []string{"child/", "second"}, expected: "http://base.com/child/second", id: "11"},
		{base: "http://base.com", parts: []string{"child/", "/second"}, expected: "http://base.com/child/second", id: "12"},
		{base: "http://base.com", parts: []string{"child/second", "third"}, expected: "http://base.com/child/second/third", id: "13"},
		{base: "http://base.com", parts: []string{"child", "second?with=args"}, expected: "http://base.com/child/second%3Fwith=args", id: "14"},
		{base: "http://base.com/yandex-api", parts: []string{"child"}, expected: "http://base.com/yandex-api/child", id: "15"},
		{base: "https://", parts: []string{"dialogs.priemka.net", "api"}, expected: "https://dialogs.priemka.net/api", id: "16"},
		{base: "https://", parts: []string{"dialogs.priemka.net", "/api"}, expected: "https://dialogs.priemka.net/api", id: "17"},
		{base: "https://", parts: []string{"quasar.yandex.ru", "iot"}, expected: "https://quasar.yandex.ru/iot", id: "18"},
		{base: "https://", parts: []string{"quasar.yandex.ru", "/iot"}, expected: "https://quasar.yandex.ru/iot", id: "19"},
		{base: "https:", parts: []string{"quasar.yandex.ru", "/iot"}, expected: "https://quasar.yandex.ru/iot", id: "20"},
		{base: "/external/v2/skills/", parts: []string{"xiaomi"}, expected: "/external/v2/skills/xiaomi", id: "21"},
	}

	for _, testCase := range cases {
		assert.Equal(t, URLJoin(testCase.base, testCase.parts...), testCase.expected, testCase.id)
	}
}
