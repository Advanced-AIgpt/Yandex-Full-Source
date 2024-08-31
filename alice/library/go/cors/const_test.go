package cors

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestYandexOriginRegexp(t *testing.T) {
	assert.Regexp(t, YandexOriginRe, "https://yandex.ru/quasar")
	assert.Regexp(t, YandexOriginRe, "http://localhost.yandex.ru:3344/")
	assert.Regexp(t, YandexOriginRe, "https://hamster-ololo.yandex.ru/skills")
	assert.Regexp(t, YandexOriginRe, "https://quasar-pull-4294-rr-templates.hamster.yandex.ru/quasar")
	assert.Regexp(t, YandexOriginRe, "https://yandex.ru/quasar")
	assert.Regexp(t, YandexOriginRe, "http://epic.yandex-team.ru:8080")

	assert.NotRegexp(t, YandexOriginRe, "https://this-is-not-yandex.ru/quasar")
	assert.NotRegexp(t, YandexOriginRe, "https://yandex.ru.ololo.com")
	assert.NotRegexp(t, YandexOriginRe, "https://yandex.rum:3344/quasar")
	assert.NotRegexp(t, YandexOriginRe, "http://yandex.ru-hacker.evil.com")
	assert.NotRegexp(t, YandexOriginRe, "https://evil.com/yandex.ru/quasar")
	assert.NotRegexp(t, YandexOriginRe, "https://more-evil.com:5999/yandex-team.ru/notquasar")
}

func TestYandexNetOriginRegexp(t *testing.T) {
	assert.Regexp(t, YandexNetOriginRe, "https://s3.mds.yandex.net/quasar-ui/sber/index.html")

	assert.NotRegexp(t, YandexNetOriginRe, "https://yandex.ru/quasar")
	assert.NotRegexp(t, YandexNetOriginRe, "http://localhost.yandex.ru:3344/")
	assert.NotRegexp(t, YandexNetOriginRe, "https://hamster-ololo.yandex.ru/skills")
	assert.NotRegexp(t, YandexNetOriginRe, "https://quasar-pull-4294-rr-templates.hamster.yandex.ru/quasar")
	assert.NotRegexp(t, YandexNetOriginRe, "https://yandex.ru/quasar")
	assert.NotRegexp(t, YandexNetOriginRe, "http://epic.yandex-team.ru:8080")
	assert.NotRegexp(t, YandexNetOriginRe, "https://yastatic.net/quasar")
	assert.NotRegexp(t, YandexNetOriginRe, "https://this-is-not-yandex.ru/quasar")
	assert.NotRegexp(t, YandexNetOriginRe, "https://yandex.ru.ololo.com")
	assert.NotRegexp(t, YandexNetOriginRe, "https://yandex.rum:3344/quasar")
	assert.NotRegexp(t, YandexNetOriginRe, "http://yandex.ru-hacker.evil.com")
	assert.NotRegexp(t, YandexNetOriginRe, "https://evil.com/yandex.ru/quasar")
	assert.NotRegexp(t, YandexNetOriginRe, "https://more-evil.com:5999/yandex-team.ru/notquasar")
}
