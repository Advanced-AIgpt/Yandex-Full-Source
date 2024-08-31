package server

import (
	"errors"
	"fmt"
	"net/http/httptest"
	"os"
	"strconv"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
)

const (
	otherTvmID = 123

	takeoutTvmID     = 111
	steelixTvmID     = 222
	timeMachineTvmID = 333
)

func TestHandlers(t *testing.T) {
	var (
		endpoint, prefix, token string
		trace                   bool
	)

	// https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic(errors.New("can not read YDB_ENDPOINT envvar"))
	}

	prefix, ok = os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic(errors.New("can not read YDB_DATABASE envvar"))
	}

	token, ok = os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	credentials := dbCredentials{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
	}
	trace = false

	suite.Run(t, &ServerSuite{
		dbCredentials:        credentials,
		historyDBCredentials: credentials,
		trace:                trace,
		takeoutTvmID:         strconv.Itoa(takeoutTvmID),
		steelixTvmID:         strconv.Itoa(steelixTvmID),
		timeMachineTvmID:     strconv.Itoa(timeMachineTvmID),
	})
}

func TestShouldBeFiltered(t *testing.T) {
	type testInput struct {
		path      string
		userAgent string
	}

	positive := []testInput{
		{
			path:      "/m/devices",
			userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_4) AppleWebKit/605.1.15 (KHTML, like Gecko)",
		},
		{
			path:      "/m/user/info",
			userAgent: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.130 Safari/537.36",
		},
		{
			path:      "/m/v3/user/devices?srcrwr=IOT_HOST:iot.quasar.yandex.ru",
			userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15",
		},
		{
			path:      "/m/mm/mmm/microwave",
			userAgent: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36",
		},
	}

	negative := []testInput{
		{
			path:      "/m/devices",
			userAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 YaBrowser/19.5.2.38.10 YaApp_iOS/78.00 YaApp_iOS_Browser/78.00 Safari/604.1 SA/3",
		},
		{
			path:      "/m/user/info",
			userAgent: "Mozilla/5.0 (Linux; arm; Android 11; Pixel 3a) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.166 YaApp_Android/21.82.1 YaSearchBrowser/21.82.1 BroPP/1.0 SA/3 Mobile Safari/537.36",
		},
		{
			path:      "/m/v3/user/devices?srcrwr=IOT_HOST:iot.quasar.yandex.ru",
			userAgent: "Mozilla/5.0 (Linux; arm_64; Android 10; Mi A2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.159 YaApp_Android/21.81.1 YaSearchBrowser/21.81.1 BroPP/1.0 SA/3 TA/7.1 Mobile Safari/537.36 IOT/1.0",
		},
		{
			path:      "/m/mm/mmm/microwave",
			userAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 YaBrowser/19.5.2.38.10 YaApp_iOS/78.00 YaApp_iOS_Browser/78.00 Safari/604.1 SA/3",
		},
		{
			path:      "/megamind/devices",
			userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_4) AppleWebKit/605.1.15 (KHTML, like Gecko)",
		},
		{
			path:      "/microwave/user/info",
			userAgent: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.130 Safari/537.36",
		},
		{
			path:      "/mama-mia/v3/user/devices?srcrwr=IOT_HOST:iot.quasar.yandex.ru",
			userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15",
		},
		{
			path:      "/a-quick-brown-fox-jumps-over-the-lazy-dog",
			userAgent: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36",
		},
		{
			path:      "/steelix/i_choose_you",
			userAgent: "Mozilla/5.0 (Linux; arm_64; Android 10; Mi A2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.159 YaApp_Android/21.81.1 YaSearchBrowser/21.81.1 BroPP/1.0 SA/3 TA/7.1 Mobile Safari/537.36 IOT/1.0",
		},
		{
			path:      "/vulpix/i_choose_you",
			userAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 YaBrowser/19.5.2.38.10 YaApp_iOS/78.00 YaApp_iOS_Browser/78.00 Safari/604.1 SA/3",
		},
	}

	for i, input := range positive {
		t.Run(fmt.Sprintf("positive_%d", i), func(t *testing.T) {
			req := httptest.NewRequest("", input.path, nil)
			req.Header.Add("User-Agent", input.userAgent)

			assert.True(t, isMobileWithUnknownAgent(req))
		})
	}

	for i, input := range negative {
		t.Run(fmt.Sprintf("negative_%d", i), func(t *testing.T) {
			req := httptest.NewRequest("", input.path, nil)
			req.Header.Add("User-Agent", input.userAgent)

			assert.False(t, isMobileWithUnknownAgent(req))
		})
	}
}
