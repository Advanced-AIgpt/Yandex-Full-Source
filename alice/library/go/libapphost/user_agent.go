package libapphost

import (
	"a.yandex-team.ru/alice/library/go/useragent"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func IsAzumaUserAgent(ctx apphost.Context) (bool, error) {
	protoRequest, err := newProtoHTTPRequest(ctx)
	if err != nil {
		return false, err
	}
	userAgent := protoRequest.userAgent()
	if useragent.IOTAppRe.MatchString(userAgent) || useragent.CentaurRe.MatchString(userAgent) {
		return true, nil
	}
	return false, xerrors.Errorf("user agent is not matched by iotAppRe or centaurRe: %q", userAgent)
}
