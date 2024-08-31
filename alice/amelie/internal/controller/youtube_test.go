package controller

import (
	"testing"

	"a.yandex-team.ru/alice/amelie/pkg/re"
)

func TestYouTubeCommandRegexp(t *testing.T) {
	for text, arg := range map[string]string{
		"/youtube":     "",
		"/youtube ":    "",
		"/youtube 123": "123",
		"/y":           "",
		"/y ":          "",
		"/y 123":       "123",
	} {
		t.Run(text, func(t *testing.T) {
			if !youTubeCommandRegexp.MatchString(text) {
				t.FailNow()
			}
			groups := re.MatchNamedGroups(youTubeCommandRegexp, text)
			actual := groups["arg"]
			if actual != arg {
				t.Errorf("expected '%s' but found '%s'", arg, actual)
			}
		})
	}
}

func TestYouTubeLinkRegexp(t *testing.T) {
	for _, link := range []string{
		"https://m.youtube.com/watch?v=ufhmEYXFunA",
	} {
		t.Run(link, func(t *testing.T) {
			if !youTubeLinkRegexp.MatchString(link) {
				t.FailNow()
			}
		})
	}
}
