package authpolicy

import (
	"fmt"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type TVMWithClientServicePolicy struct {
	Client tvm.Client
	DstID  uint32
}

func (p TVMWithClientServicePolicy) Apply(request *resty.Request) error {
	tvmServiceTicket, err := p.Client.GetServiceTicketForID(request.Context(), tvm.ClientID(p.DstID))
	if err != nil {
		return xerrors.Errorf("failed get TVM service ticket for http auth policy: %w", err)
	}
	request.SetHeaders(map[string]string{
		tvmconsts.XYaServiceTicket: tvmServiceTicket,
	})
	return nil
}

type TVMPolicy struct {
	UserTicket    string
	ServiceTicket string
}

func (p TVMPolicy) Apply(request *resty.Request) error {
	request.SetHeaders(map[string]string{
		tvmconsts.XYaServiceTicket: p.ServiceTicket,
		tvmconsts.XYaUserTicket:    p.UserTicket,
	})
	return nil
}

type TVMServiceOnlyPolicy struct {
	ServiceTicket string
}

func (p TVMServiceOnlyPolicy) Apply(request *resty.Request) error {
	request.SetHeaders(map[string]string{
		tvmconsts.XYaServiceTicket: p.ServiceTicket,
	})
	return nil
}

type AuthorizationHeaderPrefix string

const (
	// OAuthHeaderPrefix is a strange prefix is historically used by steelix, who was the first to use BlackboxOAuthUserProvider
	//
	// now all incoming clients have to send their tokens with this prefix
	// instead of widespread BearerHeaderPrefix, which is used in all outgoing events
	//
	// TL;DR
	// for all outgoing clients BearerHeaderPrefix is used
	// all incoming clients have to send OAuthHeaderPrefix
	OAuthHeaderPrefix  AuthorizationHeaderPrefix = "OAuth"
	BearerHeaderPrefix AuthorizationHeaderPrefix = "Bearer"
)

type OAuthPolicy struct {
	Prefix AuthorizationHeaderPrefix
	Token  string
}

func (p OAuthPolicy) Apply(request *resty.Request) error {
	request.SetHeaders(map[string]string{
		headers.AuthorizationKey: fmt.Sprintf("%s %s", p.Prefix, p.Token),
	})
	return nil
}
