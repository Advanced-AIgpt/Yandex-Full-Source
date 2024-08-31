package libapphost

import (
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type UserTicketExtractor func(apphostContext apphost.Context) (string, error)

func AliceScenarioUserTicketExtractor(apphostContext apphost.Context) (string, error) {
	var _ UserTicketExtractor = AliceScenarioUserTicketExtractor
	var requestMeta scenarios.TRequestMeta
	err := apphostContext.GetOnePB("mm_scenario_request_meta", &requestMeta)
	if err != nil {
		return "", err
	}
	return requestMeta.UserTicket, nil
}

func DefaultUserTicketExtractor(apphostContext apphost.Context) (string, error) {
	var _ UserTicketExtractor = DefaultUserTicketExtractor
	return apphostContext.UserTicket()
}

func TVMUserProviderMiddleware(logger log.Logger, extractor UserTicketExtractor, tvmClient tvm.Client, acl tvm.UserTicketACL) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(apphostContext apphost.Context) error {
			ctx := apphostContext.Context()
			rawUserTicket, err := extractor(apphostContext)
			if err != nil {
				return apphost.RequestErrorf("user ticket check failed: %w", err)
			}
			checkedUserTicket, err := tvmClient.CheckUserTicket(apphostContext.Context(), rawUserTicket)
			if err != nil {
				return apphost.RequestErrorf("user ticket check failed: %w", err)
			}
			if err := acl(checkedUserTicket); err != nil {
				return apphost.RequestErrorf("user ticket check failed: %w", err)
			}
			user := userctx.User{ID: uint64(checkedUserTicket.DefaultUID), Ticket: rawUserTicket}
			ctx = userctx.WithUser(ctx, user)
			ctxlog.Warnf(ctx, logger, `got raw data from tvm: {"default_uid":%d DON'T USE THIS RECORD ANYMORE`, user.ID)
			return next.ServeAppHost(apphostContext.WithContext(ctx))
		})
	}
}
