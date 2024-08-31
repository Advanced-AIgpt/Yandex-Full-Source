package uniproxy

import (
	"strings"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/session"
)

var (
	_levelsMapping = map[string]uint32{
		"without":  0,
		"medium":   1,
		"children": 2,
		"safe":     3,
	}
)

func makeFiltrationLevel(v map[string]interface{}) *uint32 {
	var current interface{} = v
	for _, part := range []string{"device_config", "content_settings"} {
		m, ok := current.(map[string]interface{})
		if ok {
			current, ok = m[part]
		}
		if !ok {
			current = nil
			break
		}
	}
	if current != nil {
		if rawLevel, ok := current.(string); ok {
			if level, ok := _levelsMapping[rawLevel]; ok {
				return &level
			}
		}
	}
	return nil
}

func getOAuthToken(account *session.AccountInfo) string {
	if account == nil {
		return ""
	}
	return account.OAuthToken
}

func makeExperiments(ctx app.Context) map[string]interface{} {
	exps := make(map[string]interface{}, len(ctx.GetSettings().GetExperiments()))
	for key, exp := range ctx.GetSettings().GetExperiments() {
		if exp.Disabled {
			continue
		}
		if exp.NumValue != nil {
			exps[key] = exp.NumValue
		} else if exp.StringValue != nil {
			exps[key] = exp.StringValue
		} else {
			exps[key] = "1"
		}
	}
	return exps
}

func escapeSuggestText(text string) string {
	return _buttonAffix + text + _buttonAffix
}

func isSuggestText(text string) bool {
	return strings.HasPrefix(text, _buttonAffix) && strings.HasSuffix(text, _buttonAffix)
}

func unescapeSuggestText(text string) string {
	return strings.ReplaceAll(text, _buttonAffix, "")
}
