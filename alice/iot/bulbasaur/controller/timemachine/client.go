package timemachine

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/time_machine/dto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"github.com/go-resty/resty/v2"
)

type Client struct {
	httpClient *resty.Client
	tvmID      tvm.ClientID
	tvm        tvm.Client
}

func NewClient(host string, tvmID tvm.ClientID, tvm tvm.Client, logger log.Logger) (*Client, error) {
	return NewClientWithResty(host, tvmID, tvm, logger, resty.New())
}

func NewClientWithResty(host string, tvmID tvm.ClientID, tvm tvm.Client, logger log.Logger, httpClient *resty.Client) (*Client, error) {
	return &Client{
		httpClient: httpClient.SetHostURL(host).OnAfterResponse(logging.GetRestyResponseLogHook(logger)),
		tvm:        tvm,
		tvmID:      tvmID,
	}, nil
}

func (c *Client) SubmitTask(ctx context.Context, submitRequest dto.TaskSubmitRequest) error {
	ticket, err := c.tvm.GetServiceTicketForID(ctx, c.tvmID)
	if err != nil {
		return err
	}

	r := c.httpClient.R().
		SetContext(ctx).
		SetHeader(tvmconsts.XYaServiceTicket, ticket).
		SetBody(submitRequest)

	res, err := r.Post("/v1.0/submit")
	if err != nil {
		return err
	}

	if res.IsError() {
		return xerrors.Errorf("time_machine: failed to submit with status code %d (%s)", res.StatusCode(), res.String())
	}

	return nil
}
