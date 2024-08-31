package proxy

import (
	"context"
	"fmt"
	"net/http"
	"net/url"
	"regexp"

	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

func newDirector(logger log.Logger, tvmClient tvm.Client, targetConfig Config) (func(*http.Request), error) {
	rewrites := buildRewrites(targetConfig)

	targetURL, err := url.Parse(targetConfig.URL)
	if err != nil {
		return nil, xerrors.Errorf("targetConfig.Url parse failed: %w", err)
	}

	addTvm := func(r *http.Request) {
		dstAlias := targetConfig.TvmAlias
		ticket, err := tvmClient.GetServiceTicketForAlias(r.Context(), dstAlias)
		if err != nil {
			directorErr := directorTVMError{TvmAlias: targetConfig.TvmAlias, Err: err}
			ctxlog.Warn(r.Context(), logger, directorErr.Error())
			panic(directorErr) // there is no other way to return error from ReverseProxy.Director
		}
		if len(ticket) > 0 {
			r.Header.Set(xYaServiceTicket, ticket)
		} else {
			r.Header.Del(xYaServiceTicket) // we should not allow impostor tickets
		}
		if userTicket := userctx.GetUserTicket(r.Context()); len(userTicket) > 0 {
			r.Header.Set(xYaUserTicket, userTicket)
		} else {
			r.Header.Del(xYaUserTicket) // we should not allow impostor tickets
		}
	}

	deleteAuthorizationHeaders := func(r *http.Request) {
		r.Header.Del("Authorization")
		r.Header.Del("Cookie")
	}

	return func(r *http.Request) {
		r.Host = targetURL.Host
		r.URL.Scheme = targetURL.Scheme
		r.URL.Host = targetURL.Host
		r.URL.Path = getPath(r.Context(), logger, rewrites, targetURL.Path, r.URL.Path)
		if targetURL.RawQuery == "" || r.URL.RawQuery == "" {
			r.URL.RawQuery = targetURL.RawQuery + r.URL.RawQuery
		} else {
			r.URL.RawQuery = targetURL.RawQuery + "&" + r.URL.RawQuery
		}
		r.Header.Set("Host", targetURL.Host)
		r.Header.Set("User-Agent", "steelix/1.0")

		if targetConfig.ShouldAddTVM() {
			addTvm(r)
		}
		if targetConfig.ShouldDeleteAuthorizationHeaders() {
			deleteAuthorizationHeaders(r)
		}
	}, nil
}

type directorTVMError struct {
	TvmAlias string
	Err      error
}

func (e directorTVMError) Error() string {
	return fmt.Sprintf("can't get TVM ticket for service %s: %s", e.TvmAlias, e.Err.Error())
}

func buildRewrites(targetConfig Config) (result []rewriteRule) {
	for _, rule := range targetConfig.Rewrites {
		re := regexp.MustCompile(rule.From)
		result = append(result, rewriteRule{
			Regexp: re,
			To:     rule.To,
		})
	}
	return result
}

type rewriteRule struct {
	Regexp *regexp.Regexp
	To     string
}

func getPath(ctx context.Context, logger log.Logger, rewrites []rewriteRule, basePath, reqPath string) string {
	ctxlog.Debugf(ctx, logger, "Using path from request: %q", reqPath)
	for i, rule := range rewrites {
		reqPath = rule.Regexp.ReplaceAllString(reqPath, rule.To)
		ctxlog.Debugf(ctx, logger, "Rewrite #%d done, path is now: %q", i+1, reqPath)
	}

	return tools.URLJoin(basePath, reqPath)
}
