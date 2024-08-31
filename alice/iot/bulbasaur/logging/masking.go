package logging

import (
	"net/url"
	"strings"
)

var rules = map[string]Rule{
	"blackbox.yandex.net": []string{"sessionid", "sslsessionid"},
}

const placeholder = "x"

type Rule []string

func (r *Rule) ProcessURL(URL string) string {
	uri, err := url.ParseRequestURI(URL)
	if err != nil {
		return URL
	}

	query := uri.Query()
	for _, param := range *r {
		if query.Get(param) != "" {
			query.Set(param, strings.Repeat(placeholder, 5))
		}
	}
	uri.RawQuery = query.Encode()
	return uri.String()
}

func GetRule(URL string) *Rule {
	parsedURL, err := url.ParseRequestURI(URL)
	if err != nil {
		return nil
	}

	rule, ok := rules[parsedURL.Host]
	if !ok {
		return nil
	}
	return &rule
}

func MaskBlackbox(URL string, response string) (string, string) {
	if rule := GetRule(URL); rule != nil {
		return rule.ProcessURL(URL), "<hidden>"
	}
	return URL, response
}

func MaskStream(URL string, response string) (string, string) {
	if strings.Contains(response, "stream_url") && strings.Contains(response, "devices.capabilities.video_stream") {
		return URL, "<hidden>"
	}
	return URL, response
}

func Mask(URL string, response string) (string, string) {
	URL, response = MaskBlackbox(URL, response)
	URL, response = MaskStream(URL, response)
	return URL, response
}
