package libapphost

import (
	"net/http"
	"strings"

	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	protoanswers "a.yandex-team.ru/apphost/lib/proto_answers"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/yandex/blackbox"
)

func BlackboxUserTicketProvider(blackboxClient blackbox.Client) apphost.HandlerFunc {
	return func(ctx apphost.Context) error {
		credentials, err := getSessionIDCredentials(ctx)
		if err != nil {
			return apphost.RequestErrorf("unable to get sessionID credentials: %w", err)
		}
		bbRequest := blackbox.SessionIDRequest{
			SessionID:     credentials.SessionID,
			UserIP:        credentials.UserIP,
			Host:          credentials.Host,
			GetUserTicket: true,
		}

		resp, err := blackboxClient.SessionID(ctx.Context(), bbRequest)
		if err != nil {
			return xerrors.Errorf("can't get user ticket from blackbox: %w", err)
		}

		ticketResponse := &protoanswers.TTvmUserTicket{
			UserTicket: resp.UserTicket,
		}
		return ctx.AddPB(apphost.TypeTVMUserTicket, ticketResponse)
	}
}

func BlackboxUserTicketProviderMiddleware(logger log.Logger, blackboxClient blackbox.Client) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) error {
			goCtx := ctx.Context()

			if credentials, err := getSessionIDCredentials(ctx); err == nil {
				bbRequest := blackbox.SessionIDRequest{
					SessionID:     credentials.SessionID,
					UserIP:        credentials.UserIP,
					Host:          credentials.Host,
					GetUserTicket: true,
				}

				if resp, err := blackboxClient.SessionID(ctx.Context(), bbRequest); err == nil {
					goCtx = userctx.WithUser(goCtx, userctx.User{ID: resp.User.ID, Login: resp.User.Login, Ticket: resp.UserTicket, IP: credentials.UserIP})
				} else {
					goCtx = userctx.WithAuthErr(goCtx, err)
					ctxlog.Warnf(goCtx, logger, "apphost: cannot authorize user: %v", err)
				}
			} else {
				goCtx = userctx.WithAuthErr(goCtx, err)
				ctxlog.Warnf(goCtx, logger, "apphost: cannot authorize user: %v", err)
			}

			ctx = ctx.WithContext(goCtx)
			return next.ServeAppHost(ctx)
		})
	}
}

func GuardAuthorizedMiddleware(logger log.Logger) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) error {
			goCtx := ctx.Context()
			if _, err := userctx.GetUser(goCtx); err != nil {
				ctxlog.Warnf(goCtx, logger, "apphost: cannot authorize user: %v", err)
				return err
			}
			return next.ServeAppHost(ctx)
		})
	}
}

type sessionIDCredentials struct {
	SessionID string
	UserIP    string
	Host      string
}

func getSessionIDCredentials(ctx apphost.Context) (sessionIDCredentials, error) {
	var credentials sessionIDCredentials

	protoRequest, err := newProtoHTTPRequest(ctx)
	if err != nil {
		return sessionIDCredentials{}, err
	}

	sessionID, err := protoRequest.cookie("Session_id")
	if err != nil {
		return credentials, xerrors.Errorf("failed to get Session_id from request cookies: %w", err)
	}

	userIP, err := protoRequest.userIP()
	if err != nil {
		return credentials, err
	}

	credentials = sessionIDCredentials{
		SessionID: sessionID.Value,
		UserIP:    userIP,
		Host:      protoRequest.host(),
	}
	return credentials, nil
}

type protoHTTPRequest struct {
	protoRequest *protoanswers.THttpRequest
	headers      http.Header
}

func newProtoHTTPRequest(ctx apphost.Context) (*protoHTTPRequest, error) {
	var request protoanswers.THttpRequest
	err := ctx.GetOnePB("proto_http_request", &request)
	if err != nil {
		return nil, err
	}
	h := http.Header{}
	for _, header := range request.GetHeaders() {
		h.Add(header.GetName(), header.GetValue())
	}
	return &protoHTTPRequest{protoRequest: &request, headers: h}, nil
}

func (p protoHTTPRequest) cookie(name string) (*http.Cookie, error) {
	r := http.Request{Header: p.headers} // you can't just read cookies from http.Header
	return r.Cookie(name)
}

func (p protoHTTPRequest) host() string {
	return p.headers.Get("Host")
}

func (p protoHTTPRequest) userAgent() string {
	return p.headers.Get(headers.UserAgentKey)
}

func (p protoHTTPRequest) userIP() (string, error) {
	if xffy := p.headers.Get("X-Forwarded-For-Y"); xffy != "" {
		return strings.Split(xffy, ",")[0], nil
	} else if xff := p.headers.Get("X-Forwarded-For"); xff != "" {
		return strings.Split(xff, ",")[0], nil
	} else if xri := p.headers.Get("X-Real-Ip"); xri != "" {
		return xri, nil
	} else if xrri := p.headers.Get("X-Real-Remote-Ip"); xrri != "" {
		return xrri, nil
	}
	return "", xerrors.New("can't find appropriate header to get ip from")
}
