package datasync

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type AuthPolicyFactory struct {
	TVMDstID uint32
	TVM      tvm.Client
}

type defaultAuthPolicy struct {
	userTicket    string
	serviceTicket string
	userID        uint64
}

func (ap defaultAuthPolicy) Apply(request *resty.Request) error {
	request.SetHeaders(map[string]string{
		tvmconsts.XYaServiceTicket: ap.serviceTicket,
		tvmconsts.XYaUserTicket:    ap.userTicket,
	})
	return nil
}

func (f AuthPolicyFactory) NewAuthPolicy(ctx context.Context, userTicket string) (authpolicy.HTTPPolicy, error) {
	tvmServiceTicket, err := f.TVM.GetServiceTicketForID(ctx, tvm.ClientID(f.TVMDstID))
	if err != nil {
		return nil, xerrors.Errorf("failed get TVM service ticket for datasync request: %w", err)
	}
	return defaultAuthPolicy{
		userTicket:    userTicket,
		serviceTicket: tvmServiceTicket,
	}, nil
}

type Client struct {
	Endpoint string

	apFactory AuthPolicyFactory
	Client    *resty.Client
	Logger    log.Logger
}

func NewClient(endpoint string, tvmClient tvm.Client, tvmDstID uint32, client *http.Client, logger log.Logger) *Client {
	return &Client{
		Endpoint:  endpoint,
		apFactory: AuthPolicyFactory{TVMDstID: tvmDstID, TVM: tvmClient},
		Client:    resty.NewWithClient(client),
		Logger:    logger,
	}
}

func (c *Client) GetAddressesForUser(ctx context.Context, userTicket string) (PersonalityAddressesResponse, error) {
	ctx = withGetAddressesForUserSignal(ctx)

	requestURL := fmt.Sprintf("%s/v2/personality/profile/addresses", c.Endpoint)
	body, err := c.simpleHTTPRequest(ctx, userTicket, http.MethodGet, requestURL, nil)
	if err != nil {
		return PersonalityAddressesResponse{}, xerrors.Errorf("unable to send get addresses for user request to datasync: %w", err)
	}

	var response PersonalityAddressesResponse
	if err := json.Unmarshal(body, &response); err != nil {
		return PersonalityAddressesResponse{}, xerrors.Errorf("unable to unmarshal get addresses for user response from datasync: %w", err)
	}
	return response, nil
}

func (c *Client) simpleHTTPRequest(ctx context.Context, userTicket string, method, url string, payload interface{}) ([]byte, error) {
	requestID := requestid.GetRequestID(ctx)
	authPolicy, err := c.apFactory.NewAuthPolicy(ctx, userTicket)
	if err != nil {
		return nil, xerrors.Errorf("unable to create auth policy from factory: %w", err)
	}
	request := c.Client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetBody(payload)
	if err := authPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("unable to apply auth policy: %w", err)
	}

	ctxlog.Info(ctx, c.Logger, "Sending request to datasync",
		log.Any("body", payload),
		log.String("url", url),
		log.String("request_id", requestID))

	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to datasync: %w", err)
	}

	body := response.Body()

	ctxlog.Info(ctx, c.Logger, "Got raw response from datasync",
		log.String("body", string(body)),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()))

	if !response.IsSuccess() {
		return nil, xerrors.Errorf("bad datasync response: status [%d], body: %s", response.StatusCode(), body)
	}
	return body, nil
}
