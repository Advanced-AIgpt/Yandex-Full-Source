package middleware

import (
	"net/http"
	"strings"

	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/alice/library/go/userip"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func GuardOAuthScope(logger log.Logger, scope string) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			ctx := r.Context()

			oauth, err := userctx.GetOAuth(ctx)
			if err != nil {
				ctxlog.Warn(ctx, logger, "user not authorized")
				http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
				return
			}
			for _, s := range oauth.Scope {
				if s == scope {
					next.ServeHTTP(w, r)
					return
				}
			}

			ctxlog.Warnf(ctx, logger, "%q not found in OAuth.Scope: %q", scope, strings.Join(oauth.Scope, " "))
			http.Error(w, http.StatusText(http.StatusForbidden), http.StatusForbidden)
		})
	}
}

type SessionIDCredentials struct {
	SessionID    string
	SSLSessionID string
	UserIP       string
}

func getSessionIDCredentials(r *http.Request) (SessionIDCredentials, error) {
	var credentials SessionIDCredentials
	sessionID, err := r.Cookie("Session_id")
	if err != nil {
		return credentials, xerrors.Errorf("failed to get Session_id from request cookies: %w", err)
	}
	userIP, err := userip.GetUserIPFromRequest(r)
	if err != nil {
		return credentials, err
	}

	credentials.SessionID = sessionID.Value
	credentials.UserIP = userIP

	sessionID2, err := r.Cookie("sessionid2")
	if err == nil {
		credentials.SSLSessionID = sessionID2.Value
	}

	return credentials, nil
}

func getOauthCredentials(r *http.Request) (string, string, error) {
	oauthToken := r.Header.Get("Authorization")
	if strings.HasPrefix(oauthToken, "OAuth ") {
		oauthToken = strings.TrimPrefix(oauthToken, "OAuth ")
	} else if strings.HasPrefix(oauthToken, "Bearer ") {
		oauthToken = strings.TrimPrefix(oauthToken, "Bearer ")
	} else {
		return "", "", xerrors.New("invalid OAuth Authorization header format")
	}

	userIP, err := userip.GetUserIPFromRequest(r)
	if err != nil {
		return "", "", xerrors.Errorf("failed to parse user ip from request: %w", err)
	}

	return oauthToken, userIP, nil
}
