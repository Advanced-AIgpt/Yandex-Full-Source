package server

import (
	"context"
	"fmt"
	"net/http"
	"strings"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type FakeResponse struct {
	header http.Header
}

func (r FakeResponse) Header() http.Header {
	return r.header
}

func (r FakeResponse) WriteHeader(n int) {
}

func (r FakeResponse) Write(b []byte) (n int, err error) {
	return len(b), nil
}

func NewFakeResponse() http.ResponseWriter {
	return FakeResponse{header: make(map[string][]string)}
}

type HandlerWithMiddlewares struct {
	Path        string
	Middlewares chi.Middlewares
}

func (suite *ServerSuite) TestContextFieldsConsistency() {
	suite.RunServerTest("ContextFieldsConsistency", func(server *TestServer, dbfiller *dbfiller.Filler) {
		handlers := make([]HandlerWithMiddlewares, 0)

		err := chi.Walk(server.server.Router, func(method string, route string, handler http.Handler, middlewares ...func(http.Handler) http.Handler) error {
			handlers = append(handlers, HandlerWithMiddlewares{
				Path:        route,
				Middlewares: middlewares,
			})
			return nil
		})
		suite.Require().NoError(err)

		routeToTVMSrcID := map[string]tvm.ClientID{
			"/takeout":       tvm.ClientID(server.server.Config.Takeout.TVMID),
			"/time_machine":  tvm.ClientID(server.server.Config.Timemachine.TVMID),
			"/v1.0/callback": tvm.ClientID(server.server.Config.Steelix.TVMID),
			"/v1.0/push":     tvm.ClientID(server.server.Config.Steelix.TVMID),
		}

		routeToOAuthClientID := map[string]*userctx.OAuth{
			"/w/user/scenarios":              {ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}},
			"/w/user/devices/speakers/calls": {ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}},
			"/w/user/devices/actions":        {ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}},
			"/w/user/devices/lighting":       {ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}},
			"/api/v1.0":                      {ClientID: "any-client-id", Scope: []string{"iot:view", "iot:control"}},
		}

		for _, handler := range handlers {
			var finalHandlerContext context.Context
			handlerFunc := handler.Middlewares.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				finalHandlerContext = r.Context()
			})

			user := model.NewUser("alice")

			request, err := http.NewRequest(http.MethodGet, "test", nil)
			suite.Require().NoError(err)

			tvmUserTicket := fmt.Sprintf("%d-%s-user-ticket", user.ID, user.Login)
			server.blackbox.User = &userctx.User{ID: user.ID, Login: user.Login, Ticket: tvmUserTicket}
			server.tvm.UserID = tvm.UID(user.ID)

			server.tvm.SrcServiceID = 100500
			for prefix, tvmID := range routeToTVMSrcID {
				if strings.HasPrefix(handler.Path, prefix) {
					server.tvm.SrcServiceID = tvmID
				}
			}

			for prefix, oauthToken := range routeToOAuthClientID {
				if strings.HasPrefix(handler.Path, prefix) {
					server.blackbox.OAuthToken = oauthToken
				}
			}

			request.Header.Set(tvmconsts.XYaServiceTicket, "default-service-ticket-value")
			request.Header.Set("Cookie", "Session_id=test_session_id_cookie;sessionid2=test_sessionid2_cookie")
			request.Header.Set("X-Forwarded-For-Y", "127.0.0.1")
			request.Header.Set("Authorization", "OAuth test_oauth_token")
			request.Header.Set(tvmconsts.XYaUserTicket, tvmUserTicket)

			handlerFunc.ServeHTTP(NewFakeResponse(), request)
			suite.NotNil(finalHandlerContext, "Failed to reach final handler for path `%s`, check its middlewares", handler.Path)
			// this error means that we have not reached the final handler func (we stopped at some middleware)
			// but here we check that the final request context fields are the same as root request context fields

			initialRequestContextFields := ctxlog.ContextFields(request.Context())
			finalRequestContextFields := ctxlog.ContextFields(finalHandlerContext)

			suite.EqualValues(finalRequestContextFields, initialRequestContextFields, "Failed handler: %s", handler.Path)
			// this error means that we have middleware that copies request and pass it to the chain further
			// if we want to add some fields in the deeper middleware, we won't have access to these fields in outer
			// logging middleware
			// in order to avoid that, in middlewares we use this workaround:
			//
			//	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			//		ctx := r.Context()
			//		user := User{ID: uint64(parsedUserTicket.DefaultUID), Ticket: ticket}
			//		ctx = contextWithUser(ctx, user)
			//		*r = *r.WithContext(ctx)
			//		next.ServeHTTP(w, r)
			//	})
		}
	})
}
