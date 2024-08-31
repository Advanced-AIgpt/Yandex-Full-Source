package cors

import (
	"regexp"
)

var (
	YandexOriginRe    = regexp.MustCompile(`^http(s)?://([a-z0-9_-]+\.)*yandex(-team)?\.(ru|com)(:|/|$)`)
	YandexNetOriginRe = regexp.MustCompile(`^http(s)?://([a-z0-9_-]+\.)*yandex\.net(/|$)`)

	AllowYandexOriginFunc = func(origin string) bool {
		return len(origin) > 0 && YandexOriginRe.MatchString(origin)
	}
)
