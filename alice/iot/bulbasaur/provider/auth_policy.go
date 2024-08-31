package provider

import (
	"fmt"
	"strconv"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/library/go/httputil/headers"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
)

var (
	XFunctionsServiceTicket = "X-Functions-Service-Ticket"
)

type HTTPAuthPolicyFunc func(*resty.Request) error

func (authPolicy HTTPAuthPolicyFunc) Apply(request *resty.Request) error { return authPolicy(request) }

func NewOAuthPolicy(token string) authpolicy.HTTPPolicy {
	return authpolicy.OAuthPolicy{
		Prefix: authpolicy.BearerHeaderPrefix,
		Token:  token,
	}
}

func NewInternalAuthPolicy(tvmServiceTicket string, userID uint64) authpolicy.HTTPPolicy {
	return HTTPAuthPolicyFunc(func(request *resty.Request) error {
		request.SetHeaders(map[string]string{
			tvmconsts.XYaServiceTicket:           tvmServiceTicket,
			adapter.InternalProviderUserIDHeader: strconv.FormatUint(userID, 10),
		})
		return nil
	})
}

func NewXiaomiAuthPolicy(token string, userID uint64) authpolicy.HTTPPolicy {
	return HTTPAuthPolicyFunc(func(request *resty.Request) error {
		request.SetHeaders(map[string]string{
			headers.AuthorizationKey:             fmt.Sprintf("%s %s", authpolicy.BearerHeaderPrefix, token),
			adapter.InternalProviderUserIDHeader: strconv.FormatUint(userID, 10),
		})
		return nil
	})
}

func NewQuasarAuthPolicy(tvmUserTicket, tvmServiceTicket string, userID uint64) authpolicy.HTTPPolicy {
	return HTTPAuthPolicyFunc(func(request *resty.Request) error {
		request.SetHeaders(map[string]string{
			tvmconsts.XYaUserTicket:              tvmUserTicket,
			tvmconsts.XYaServiceTicket:           tvmServiceTicket,
			adapter.InternalProviderUserIDHeader: strconv.FormatUint(userID, 10),
		})
		return nil
	})
}

func NewCloudFunctionsAuthPolicy(tvmTicket string) authpolicy.HTTPPolicy {
	return HTTPAuthPolicyFunc(func(request *resty.Request) error {
		request.SetHeaders(map[string]string{
			XFunctionsServiceTicket: tvmTicket,
		})
		return nil
	})
}

type RPCAuthPolicy interface {
	Apply(*adapter.JSONRPCRequest)
}

type RPCAuthPolicyFunc func(r *adapter.JSONRPCRequest)

func (authPolicy RPCAuthPolicyFunc) Apply(request *adapter.JSONRPCRequest) { authPolicy(request) }

func NewCloudFunctionsRPCOAuthPolicy(token string) RPCAuthPolicy {
	return RPCAuthPolicyFunc(func(r *adapter.JSONRPCRequest) {
		// these are not http headers but rather body values - cloud functions does not support http headers
		// https://st.yandex-team.ru/CLOUD-30363
		r.Headers.Authorization = fmt.Sprintf("Bearer %s", token)
	})
}
