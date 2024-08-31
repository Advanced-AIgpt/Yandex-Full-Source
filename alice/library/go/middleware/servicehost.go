package middleware

import (
	"net/http"
	"strings"

	"a.yandex-team.ru/alice/library/go/servicehost"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func NewServiceHostMiddleware(logger log.Logger, serviceKey string) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			ctx := r.Context()
			// https://st.yandex-team.ru/QUASARUI-163
			if hosts, ok := r.URL.Query()["srcrwr"]; ok {
				for _, hostInfo := range hosts {
					if strings.HasPrefix(hostInfo, serviceKey) {
						parsedInfo := strings.Split(hostInfo, ":")
						if len(parsedInfo) != 2 {
							ctxlog.Warnf(ctx, logger, "Wrong srcrwr %s value format: should be %s:url", serviceKey, serviceKey)
							continue
						}
						url := parsedInfo[1]
						ctxlog.Infof(ctx, logger, "Added custom service host to context: %s = %s", serviceKey, url)
						ctx = servicehost.WithServiceHostURL(ctx, serviceKey, url)
					}
				}
			}
			*r = *r.WithContext(ctx)
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}
