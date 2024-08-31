package memento

import (
	"context"
	"net/http"

	"github.com/go-resty/resty/v2"
	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
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
		return nil, xerrors.Errorf("failed get TVM service ticket for memento request: %w", err)
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

func (c *Client) GetUserObjects(ctx context.Context, userTicket string, request *memento.TReqGetUserObjects) (*memento.TRespGetUserObjects, error) {
	ctx = withGetUserObjectsSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/get_objects")

	serialized, err := proto.Marshal(request)
	if err != nil {
		return nil, xerrors.Errorf("unable to serialize get user objects request: %w", err)
	}
	body, err := c.simpleHTTPRequest(ctx, http.MethodPost, url, userTicket, serialized)
	if err != nil {
		return nil, xerrors.Errorf("unable to send get user objects request to memento: %w", err)
	}

	var response memento.TRespGetUserObjects
	if err := proto.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal get user objects response to memento: %w", err)
	}
	return &response, nil
}

func (c *Client) UpdateUserObjects(ctx context.Context, userTicket string, request *memento.TReqChangeUserObjects) (*memento.TRespChangeUserObjects, error) {
	ctx = withUpdateUserObjectsSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/update_objects")

	serialized, err := proto.Marshal(request)
	if err != nil {
		return nil, xerrors.Errorf("unable to serialize update user objects request: %w", err)
	}
	body, err := c.simpleHTTPRequest(ctx, http.MethodPost, url, userTicket, serialized)
	if err != nil {
		return nil, xerrors.Errorf("unable to send update user objects request to memento: %w", err)
	}

	var response memento.TRespChangeUserObjects
	if err := proto.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal update user objects response to memento: %w", err)
	}
	return &response, nil
}

func (c *Client) simpleHTTPRequest(ctx context.Context, method, url, ticket string, payload interface{}) ([]byte, error) {
	requestID := requestid.GetRequestID(ctx)
	authPolicy, err := c.apFactory.NewAuthPolicy(ctx, ticket)
	if err != nil {
		return nil, xerrors.Errorf("unable to create auth policy from factory: %w", err)
	}
	request := c.Client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetHeader(tvmconsts.XYaUserTicket, ticket).
		SetHeader(headers.ContentTypeKey, headers.TypeApplicationProtobuf.String()).
		SetBody(payload)
	if err := authPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("unable to apply auth policy: %w", err)
	}

	ctxlog.Info(ctx, c.Logger, "Sending request to memento",
		log.Any("body", payload),
		log.String("url", url),
		log.String("request_id", requestID))

	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to memento: %w", err)
	}

	body := response.Body()

	ctxlog.Info(ctx, c.Logger, "Got raw response from memento",
		log.String("body", string(body)),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()))

	if !response.IsSuccess() {
		return nil, xerrors.Errorf("bad memento response: status [%d], body: %s", response.StatusCode(), body)
	}
	return body, nil
}
