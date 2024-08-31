package middleware

import (
	"net/http"
	"strconv"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type UserExtractor func(w http.ResponseWriter, r *http.Request) (userctx.User, error)

func NewBlackboxSessionIDUserExtractor(logger log.Logger, bb blackbox.Client) UserExtractor {
	return func(w http.ResponseWriter, r *http.Request) (userctx.User, error) {
		ctx := r.Context()
		credentials, err := getSessionIDCredentials(r)
		if err != nil {
			return userctx.User{}, err
		}
		bbRequest := blackbox.SessionIDRequest{
			SessionID:     credentials.SessionID,
			UserIP:        credentials.UserIP,
			Host:          r.Host,
			Aliases:       []blackbox.UserAlias{blackbox.UserAliasYandexoid},
			GetUserTicket: true,
		}
		resp, err := bb.SessionID(ctx, bbRequest)
		if err != nil {
			return userctx.User{}, err
		}
		return userctx.User{
			ID:     resp.User.ID,
			Login:  resp.User.Login,
			Ticket: resp.UserTicket,
			IP:     credentials.UserIP,
		}, nil
	}
}

func NewBlackboxOAuthUserExtractor(logger log.Logger, bb blackbox.Client, clientIDs ...string) UserExtractor {
	return func(w http.ResponseWriter, r *http.Request) (userctx.User, error) {
		ctx := r.Context()
		oauthToken, userIP, err := getOauthCredentials(r)
		if err != nil {
			return userctx.User{}, err
		}
		bbRequest := blackbox.OAuthRequest{
			OAuthToken:    oauthToken,
			UserIP:        userIP,
			Aliases:       []blackbox.UserAlias{blackbox.UserAliasYandexoid},
			GetUserTicket: true,
		}
		resp, err := bb.OAuth(ctx, bbRequest)
		if err != nil {
			return userctx.User{}, err
		}
		oauth := userctx.OAuth{ClientID: resp.ClientID, Scope: resp.Scopes}
		if len(clientIDs) > 0 && !slices.Contains(clientIDs, oauth.ClientID) {
			return userctx.User{}, xerrors.Errorf("OAuth.ClientId mismatch: got %s, expected one of [%s]",
				oauth.ClientID, strings.Join(clientIDs, ", "))
		}
		ctx = userctx.WithOAuth(ctx, oauth)
		*r = *r.WithContext(ctx)
		return userctx.User{ID: resp.User.ID, Login: resp.User.Login, Ticket: resp.UserTicket}, nil
	}
}

func NewTvmUserExtractor(logger log.Logger, t tvm.Client) UserExtractor {
	return func(w http.ResponseWriter, r *http.Request) (userctx.User, error) {
		ctx := r.Context()
		ticket := r.Header.Get(xYaUserTicket)
		if len(ticket) == 0 {
			err := xerrors.Errorf("failed to check user ticket: %s header is empty", xYaUserTicket)
			ctxlog.Warnf(ctx, logger, err.Error())
			return userctx.User{}, err
		}
		parsedUserTicket, err := t.CheckUserTicket(ctx, ticket)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "failed to check user ticket: %v", err)
			return userctx.User{}, err
		}
		ctxlog.Warnf(ctx, logger, `got raw data from tvm: {"default_uid":%d DON'T USE THIS RECORD ANYMORE`, parsedUserTicket.DefaultUID)
		return userctx.User{ID: uint64(parsedUserTicket.DefaultUID), Ticket: ticket}, nil
	}
}

func NewHeaderUserExtractor(logger log.Logger, headerName string) UserExtractor {
	return func(w http.ResponseWriter, r *http.Request) (userctx.User, error) {
		ctx := r.Context()
		headerValue := r.Header.Get(headerName)
		if len(headerValue) == 0 {
			return userctx.User{}, xerrors.Errorf("cannot authorize user: %s header is empty", headerName)
		}
		uid, err := strconv.ParseUint(headerValue, 10, 0)
		if err != nil {
			return userctx.User{}, xerrors.Errorf("cannot authorize user: failed to parse value of %s header: %v", headerName, err)
		}
		ctxlog.Warnf(ctx, logger, `got user UID in %s header: %d`, headerName, uid)
		return userctx.User{ID: uid}, nil
	}
}
