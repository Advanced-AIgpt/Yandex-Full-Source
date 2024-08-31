package socialism

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
	"strconv"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	consumer Consumer
	endpoint string
	logger   log.Logger
	client   *resty.Client
}

func NewClient(consumer Consumer, endpoint string, logger log.Logger) *Client {
	return &Client{
		consumer: consumer,
		endpoint: endpoint,
		logger:   logger,
		client:   resty.New(),
	}
}

func NewClientWithResty(consumer Consumer, endpoint string, restyClient *resty.Client, logger log.Logger, opts ...RestyOption) *Client {
	for _, opt := range opts {
		restyClient = opt(restyClient)
	}
	c := &Client{
		consumer: consumer,
		endpoint: endpoint,
		logger:   logger,
		client:   restyClient,
	}
	return c
}

func (s *Client) GetUserApplicationToken(ctx context.Context, uid uint64, skillInfo SkillInfo) (string, error) {
	tokenInfo, err := s.getUserApplicationTokenInfo(ctx, uid, skillInfo)
	if err != nil {
		return "", err
	}
	return tokenInfo.Value, nil
}

func (s *Client) getUserApplicationTokenInfo(ctx context.Context, uid uint64, skillInfo SkillInfo) (*tokenInfo, error) {
	ctxlog.Infof(ctx, s.logger, "getting user token for application %s", skillInfo.applicationName)
	ctx = withGetUserApplicationTokenInfoSignal(ctx, skillInfo)
	urlParams := url.Values{}
	urlParams.Add("uid", strconv.FormatUint(uid, 10))
	urlParams.Add("application_name", skillInfo.applicationName)
	urlParams.Add("consumer", s.consumer.String())
	requestURL := tools.URLJoin(s.endpoint, "/token/newest")

	response, err := s.simpleHTTPRequest(ctx, http.MethodGet, requestURL, urlParams, nil)
	if err != nil {
		return nil, xerrors.Errorf("failed to get user application token from Social API: %w", err)
	}
	if statusCode := response.StatusCode(); statusCode != 200 {
		switch statusCode {
		case 404:
			return nil, &TokenNotFoundError{}
		default:
			return nil, xerrors.Errorf("failed to get user application token from Social API: [%d] %s", statusCode, response.Status())
		}
	}
	body := response.Body()
	var responseBody struct {
		Token *tokenInfo    `json:"token"`
		Error errorResponse `json:"error"`
	}
	if err := json.Unmarshal(body, &responseBody); err != nil {
		return nil, xerrors.Errorf("user application token request from social API failed. Failed to parse response json: %q: %w", body, err)
	}

	if !responseBody.Error.Empty() {
		return nil, xerrors.Errorf("user application token request from social API failed. Reason: %q", responseBody.Error.Description)
	}
	return responseBody.Token, nil
}

func (s *Client) DeleteUserToken(ctx context.Context, uid uint64, skillInfo SkillInfo) error {
	ctxlog.Infof(ctx, s.logger, "deleting user token for application %s", skillInfo.applicationName)

	ctx = withDeleteUserTokenSignal(ctx, skillInfo)
	requestURL := tools.URLJoin(s.endpoint, "/token/delete_from_account")

	payload := fmt.Sprintf("uid=%s&application_name=%s", strconv.FormatUint(uid, 10), skillInfo.applicationName)
	response, err := s.simpleHTTPRequest(ctx, http.MethodPost, requestURL, nil, payload)
	if err != nil {
		return xerrors.Errorf("failed to delete token from Social API: %w", err)
	}
	if statusCode := response.StatusCode(); statusCode != 200 {
		return xerrors.Errorf("failed to delete user application token from Social API: [%d] %s", statusCode, response.Status())
	}
	return nil
}

func (s *Client) CheckUserAppTokenExists(ctx context.Context, uid uint64, skillInfo SkillInfo) (bool, error) {
	ctxlog.Infof(ctx, s.logger, "checking user token for application %s", skillInfo.applicationName)
	ctx = withCheckUserAppTokenExistsSignal(ctx, skillInfo)
	urlParams := url.Values{}
	urlParams.Add("uid", strconv.FormatUint(uid, 10))
	urlParams.Add("application_names", skillInfo.applicationName)
	urlParams.Add("consumer", s.consumer.String())
	requestURL := tools.URLJoin(s.endpoint, "/token/available")

	response, err := s.simpleHTTPRequest(ctx, http.MethodGet, requestURL, urlParams, nil)
	if err != nil {
		return false, xerrors.Errorf("failed to get user application token availability info from Social API: %w", err)
	}
	if statusCode := response.StatusCode(); statusCode != 200 {
		return false, xerrors.Errorf("failed to get user application token availability info from Social API: [%d] %s", statusCode, response.Status())
	}
	body := response.Body()
	type application struct {
		Name string `json:"application_name"`
	}
	responseBody := make([]application, 0, 1)
	if err := json.Unmarshal(body, &responseBody); err != nil {
		return false, xerrors.Errorf("user application token availability info request from social API failed. Failed to parse response json: %q: %w", body, err)
	}

	for _, app := range responseBody {
		if app.Name == skillInfo.applicationName {
			return true, nil
		}
	}
	return false, nil
}

func (s *Client) simpleHTTPRequest(ctx context.Context, method, url string, urlValues url.Values, payload interface{}) (*resty.Response, error) {
	requestID := requestid.GetRequestID(ctx)
	request := s.client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetHeader("Content-Type", "application/x-www-form-urlencoded").
		SetQueryParamsFromValues(urlValues).
		SetBody(payload)

	ctxlog.Info(ctx, s.logger,
		"Sending request to socialism",
		log.String("method", method),
		log.String("url", url),
		log.String("request_id", requestID),
	)
	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to socialism: %w", err)
	}

	ctxlog.Info(ctx, s.logger,
		"Got response from socialism",
		log.String("method", method),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()),
	)
	return response, nil
}
