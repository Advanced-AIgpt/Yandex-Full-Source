package notificator

import (
	"context"
	"net/http"

	"github.com/go-resty/resty/v2"
	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	matrixpb "a.yandex-team.ru/alice/protos/api/matrix"
	notificatorpb "a.yandex-team.ru/alice/protos/api/notificator"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Client struct {
	Endpoint string

	AuthPolicy authpolicy.TVMWithClientServicePolicy
	Client     *resty.Client
	Logger     log.Logger
}

func NewClient(endpoint string, tvmClient tvm.Client, tvmDstID uint32, client *http.Client, logger log.Logger) *Client {
	return &Client{
		Endpoint:   endpoint,
		AuthPolicy: authpolicy.TVMWithClientServicePolicy{DstID: tvmDstID, Client: tvmClient},
		Client:     resty.NewWithClient(client),
		Logger:     logger,
	}
}

func (c *Client) SendDeliveryPush(ctx context.Context, request *matrixpb.TDelivery) (*matrixpb.TDeliveryResponse, error) {
	ctx = withSendTypedSemanticFramePushSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/delivery/push")

	serialized, err := proto.Marshal(request)
	if err != nil {
		return nil, xerrors.Errorf("unable to serialize delivery request to notificator: %w", err)
	}
	body, err := c.simpleHTTPRequest(ctx, http.MethodPost, url, serialized)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to notificator: %w", err)
	}

	var response matrixpb.TDeliveryResponse
	if err := proto.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal delivery response from notificator: %w", err)
	}

	ctxlog.Info(ctx, c.Logger, "sent delivery push to notificator",
		log.String("device_id", request.GetDeviceId()),
		log.String("push_id", response.GetPushId()),
	)

	return &response, nil
}

func (c *Client) GetDevices(ctx context.Context, request *notificatorpb.TGetDevicesRequest) (*notificatorpb.TGetDevicesResponse, error) {
	ctx = withGetDevicesSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/devices")

	serialized, err := proto.Marshal(request)
	if err != nil {
		return nil, xerrors.Errorf("unable to serialize getDevices request to notificator: %w", err)
	}
	body, err := c.simpleHTTPRequest(ctx, http.MethodPost, url, serialized) // you thought this handler is GET? it's not.
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to notificator: %w", err)
	}

	var response notificatorpb.TGetDevicesResponse
	if err := proto.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal getDevices response from notificator: %w", err)
	}
	return &response, nil
}

func (c *Client) simpleHTTPRequest(ctx context.Context, method, url string, payload interface{}) ([]byte, error) {
	requestID := requestid.GetRequestID(ctx)
	request := c.Client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetHeader(headers.ContentTypeKey, headers.TypeApplicationProtobuf.String()).
		SetBody(payload)
	if err := c.AuthPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("failed to apply http auth policy: %w", err)
	}

	ctxlog.Info(ctx, c.Logger, "Sending request to notificator",
		log.Any("body", payload),
		log.String("url", url),
		log.String("request_id", requestID))

	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to notificator: %w", err)
	}

	body := response.Body()

	ctxlog.Info(ctx, c.Logger, "Got raw response from notificator",
		log.String("body", string(body)),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()))

	if !response.IsSuccess() {
		return nil, xerrors.Errorf("bad notificator response: status [%d], body: %s", response.StatusCode(), body)
	}
	return body, nil
}
