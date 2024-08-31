package geosuggest

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	Endpoint string

	ClientID string
	Client   *resty.Client
	Logger   log.Logger
}

func NewClient(endpoint, clientID string, client *http.Client, logger log.Logger) *Client {
	return &Client{
		Endpoint: endpoint,
		Client:   resty.NewWithClient(client),
		ClientID: clientID,
		Logger:   logger,
	}
}

func (c *Client) GetGeosuggestFromAddress(ctx context.Context, address string) (GeosuggestFromAddressResponse, error) {
	ctx = withGetGeosuggestFromAddressSignal(ctx)

	suggestURL := fmt.Sprintf("%s/suggest-geo", c.Endpoint)
	queryParams := map[string]string{
		"part": address,
	}
	body, err := c.simpleHTTPRequest(ctx, http.MethodGet, suggestURL, nil, queryParams)
	if err != nil {
		return GeosuggestFromAddressResponse{}, xerrors.Errorf("unable to send get geosuggest from address request to geosuggest: %w", err)
	}

	var response GeosuggestFromAddressResponse
	if err := json.Unmarshal(body, &response); err != nil {
		return GeosuggestFromAddressResponse{}, xerrors.Errorf("unable to unmarshal geosuggest from address response from geosuggest: %w", err)
	}
	return response, nil
}

func (c *Client) simpleHTTPRequest(ctx context.Context, method, url string, payload interface{}, queryParams map[string]string) ([]byte, error) {
	requestID := requestid.GetRequestID(ctx)
	// documentation: https://a.yandex-team.ru/arc/trunk/arcadia/geosuggest/README.md
	// example: https://st.yandex-team.ru/IOT-786#603fa4ca27fa8c6611a5351c
	basicQueryParams := map[string]string{
		"callback":    "",
		"search_type": "addr",  // narrows the searching type to only addresses, not searching for organisations
		"v":           "9",     // handler version, different version has different API
		"n":           "5",     // number of suggests in response
		"lang":        "ru_RU", // language of suggests names in response
		"add_coords":  "1",     // flag to show geocoords in suggests
		"client_id":   c.ClientID,
	}
	request := c.Client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetBody(payload).
		SetQueryParams(basicQueryParams).
		SetQueryParams(queryParams)

	ctxlog.Info(ctx, c.Logger, "Sending request to geosuggest",
		log.Any("body", payload),
		log.String("url", url),
		log.String("request_id", requestID))

	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to geosuggest: %w", err)
	}

	body := response.Body()

	ctxlog.Info(ctx, c.Logger, "Got raw response from geosuggest",
		log.String("body", string(body)),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()))

	if !response.IsSuccess() {
		return nil, xerrors.Errorf("bad geosuggest response: status [%d], body: %s", response.StatusCode(), body)
	}
	return body, nil
}
