package useragent

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestIOTAppRe(t *testing.T) {
	testCases := []string{
		`IOT/1.0`,
		`IOT/Test/1.0`,
		`AnyText IOT/1.0`,
		`IOT/Test/1.0 AnyText`,
		`AnyText IOT/1.0 AnyText`,
		`Mozilla/5.0 (iPhone; CPU iPhone OS 14_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0 YaBrowser/21.6.4.666.10 SA/3 IOT/1.0 Mobile/15E148 Safari/604.1`,
		`Mozilla/5.0 (iPhone; CPU iPhone OS 14_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0 YaBrowser/21.6.4.666.10 SA/3 IOT/Test/1.0 Mobile/15E148 Safari/604.1`,
		`Mozilla/5.0 (Linux; arm; Android 11; SM-A525F) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.164 YaApp_Android/21.65.1 YaSearchBrowser/21.65.1 BroPP/1.0 SA/3 TA/7.1 Mobile Safari/537.36 IOT/Test/1.0`,
	}
	for _, testCase := range testCases {
		assert.True(t, IOTAppRe.MatchString(testCase))
	}
}

func TestCentaurRe(t *testing.T) {
	testCases := []string{
		`centaur/8701225.master`,
		`centaur/users/galecore/branchname`,
		`centaur/9000shrap_nel-13`,
		`centaur/releases/smartdevices/centaur-15`,
		`centaur/8701225`,
		`centaur/8701225.trunk`,
		`centaur/trunk`,
		`AnyText centaur/trunk`,
		`centaur/trunk AnyText`,
		`AnyText centaur/trunk AnyText`,
	}
	for _, testCase := range testCases {
		assert.Truef(t, CentaurRe.MatchString(testCase), `failed to match using CentaurRe: %q`, testCase)
	}
}

func TestSearchPortalRe(t *testing.T) {
	positive := []string{
		"Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 YaBrowser/19.5.2.38.10 YaApp_iOS/78.00 YaApp_iOS_Browser/78.00 Safari/604.1 SA/3",
		"Mozilla/5.0 (Linux; arm; Android 11; Pixel 3a) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.166 YaApp_Android/21.82.1 YaSearchBrowser/21.82.1 BroPP/1.0 SA/3 Mobile Safari/537.36",
		"Mozilla/5.0 (Linux; arm_64; Android 10; Mi A2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.159 YaApp_Android/21.81.1 YaSearchBrowser/21.81.1 BroPP/1.0 SA/3 TA/7.1 Mobile Safari/537.36 IOT/1.0",
	}

	negative := []string{
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.88 Safari/537.36",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.102 Safari/537.36 Edge/18.18363",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.149 Safari/537.36",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 14_4_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0.3 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 14_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0.3 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 13_1_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.1 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.212 Safari/537.36",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 14_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0.1 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 10_0 like Mac OS X) AppleWebKit/602.4.6 (KHTML, like Gecko) Version/10.0 Mobile/14A346 Safari/E7FBAF",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 14_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0.2 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 13_4_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (Windows NT 5.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.90 Safari/537.36",
		"Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.90 Safari/537.36",
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_4) AppleWebKit/605.1.15 (KHTML, like Gecko)",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.130 Safari/537.36",
		"Mozilla/5.0 (iPad; CPU OS 9_3_5 like Mac OS X) AppleWebKit/601.1.46 (KHTML, like Gecko) Mobile/13G36",
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.93 Safari/537.36",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 12_4_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/12.1.2 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36",
		"Mozilla/4.0 (compatible; MSIE 6.0; Windows 98)",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36 Edge/15.15063",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 14_7_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.2 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 12_3_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/12.1.1 Mobile/15E148 Safari/604.1",
		"Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.113 Safari/537.36",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.88 Safari/537.36",
		"Mozilla/5.0 (Windows NT 6.3; WOW64; Trident/7.0; rv:11.0) like Gecko",
		"Mozilla/5.0 (compatible; YandexBot/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 8_1 like Mac OS X) AppleWebKit/600.1.4 (KHTML, like Gecko) Version/8.0 Mobile/12B411 Safari/600.1.4 (compatible; YandexBot/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexAccessibilityBot/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (iPhone; CPU iPhone OS 8_1 like Mac OS X) AppleWebKit/600.1.4 (KHTML, like Gecko) Version/8.0 Mobile/12B411 Safari/600.1.4 (compatible; YandexMobileBot/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexDirectDyn/1.0; +http://yandex.com/bots",
		"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36 (compatible; YandexScreenshotBot/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexImages/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexVideo/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexVideoParser/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexMedia/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexBlogs/0.99; robot; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexFavicons/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexWebmaster/2.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexPagechecker/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexImageResizer/2.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexAdNet/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexDirect/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YaDirectFetcher/1.0; Dyatel; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexCalendar/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexSitelinks; Dyatel; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexMetrika/2.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexNews/4.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexCatalog/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexMarket/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexVertis/3.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexForDomain/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexBot/3.0; MirrorDetector; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexSpravBot/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexSearchShop/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36 (compatible; YandexMedianaBot/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexOntoDB/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexOntoDBAPI/1.0; +http://yandex.com/bots)",
		"Mozilla/5.0 (compatible; YandexVerticals/1.0; +http://yandex.com/bots)",
	}

	for i, userAgent := range positive {
		t.Run(fmt.Sprintf("positive_%d", i), func(t *testing.T) {
			assert.True(t, SearchPortalRe.MatchString(userAgent))
		})
	}

	for i, userAgent := range negative {
		t.Run(fmt.Sprintf("negative_%d", i), func(t *testing.T) {
			assert.False(t, SearchPortalRe.MatchString(userAgent))
		})
	}
}
