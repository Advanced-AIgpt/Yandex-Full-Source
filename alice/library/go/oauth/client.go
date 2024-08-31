package oauth

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/go-resty/resty/v2"
	"net/http"
	"strings"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	okResponseStatus = "ok"
)

type client struct {
	restyClient       *resty.Client
	authPolicyFactory authpolicy.TVMFactory
	consumer          string
}

func NewClient(
	baseURL string,
	consumer string,
	tvmPolicyFactory authpolicy.TVMFactory,
	opts ...ClientOption,
) Client {
	restyClient := resty.NewWithClient(&http.Client{})
	restyClient.SetBaseURL(baseURL)

	for _, opt := range opts {
		opt(restyClient)
	}

	return &client{
		authPolicyFactory: tvmPolicyFactory,
		restyClient:       restyClient,
		consumer:          consumer,
	}
}

func (c *client) IssueAuthorizationCode(
	ctx context.Context,
	userData UserData,
	params IssueAuthorizationCodeParams,
) (AuthorizationCode, error) {
	req := c.restyClient.R().
		SetContext(withSignal(ctx, issueAuthorizationCodeSignal)).
		SetFormData(params.toFormData(c.consumer)).
		SetHeader(headerConsumerClientIP, userData.IP)

	tvmPolicy, err := c.authPolicyFactory.NewAuthPolicy(ctx, userData.UserTicket)
	if err != nil {
		return AuthorizationCode{}, xerrors.Errorf("failed to create tvm policy: %w", err)
	}

	if err = tvmPolicy.Apply(req); err != nil {
		return AuthorizationCode{}, xerrors.Errorf("failed to apply tvm policy: %w", err)
	}

	resp, err := req.Post("/api/1/authorization_code/issue")
	if err != nil {
		return AuthorizationCode{}, xerrors.Errorf("failed to issue authorization_code: %w", err)
	}

	if !resp.IsSuccess() {
		return AuthorizationCode{}, xerrors.Errorf("request %s returned non success status code %d, (%s)",
			req.URL, resp.StatusCode(), resp.Body())
	}

	var responseBody IssueAuthorizationCodeResponse
	if err := json.Unmarshal(resp.Body(), &responseBody); err != nil {
		return AuthorizationCode{}, xerrors.Errorf("failed to unmarshal body from response on %s: %w", req.URL, err)
	}

	if responseBody.Status != okResponseStatus {
		return AuthorizationCode{}, xerrors.New(fmt.Sprintf("api return non ok status: %s, errors: %s",
			responseBody.Status, strings.Join(responseBody.Errors, ",")))
	}

	return AuthorizationCode{
		Code:      responseBody.Code,
		ExpiresIn: responseBody.ExpiresIn,
	}, nil
}
