package passport

import (
	"context"
	"fmt"
	"net/url"

	"github.com/go-resty/resty/v2"
)

const (
	oauthURL = "https://oauth.yandex.ru"
)

type OAuthClient interface {
	GenerateCodeURL() string
	GenerateOAuthCallbackURL(bot string) string
	GetOAuthToken(ctx context.Context, code string) (OAuthResponse, error)
	RefreshOAuthToken(ctx context.Context, refreshToken string) (OAuthResponse, error)
}

type OAuthResponse struct {
	TokenType    string `json:"token_type"`
	AccessToken  string `json:"access_token"`
	ExpiresIn    int64  `json:"expires_in"`
	RefreshToken string `json:"refresh_token"`
}

type oauthClient struct {
	client       *resty.Client
	clientID     string
	clientSecret string
}

func New(client *resty.Client, clientID string, clientSecret string) OAuthClient {
	return &oauthClient{client: client, clientID: clientID, clientSecret: clientSecret}
}

func (client *oauthClient) GenerateCodeURL() string {
	return fmt.Sprintf("%s/authorize?response_type=code&client_id=%s&force_confirm=yes", oauthURL, client.clientID)
}

func (client *oauthClient) GenerateOAuthCallbackURL(bot string) string {
	u, _ := url.Parse(client.GenerateCodeURL())
	q := u.Query()
	q.Add("redirect_uri", "https://d5d8gi1f8dtb38ooubk8.apigw.yandexcloud.net/oauth/token")
	q.Add("state", bot)
	u.RawQuery = q.Encode()
	return u.String()
}

func (client *oauthClient) GetOAuthToken(ctx context.Context, code string) (response OAuthResponse, err error) {
	r, err := client.client.R().
		SetBasicAuth(client.clientID, client.clientSecret).
		SetFormData(map[string]string{
			"code":       code,
			"grant_type": "authorization_code",
		}).
		SetResult(&response).
		SetContext(ctx).
		Post(oauthURL + "/token")
	if err != nil {
		return response, fmt.Errorf("oauth call error: %w", err)
	}
	if r.IsSuccess() {
		return response, nil
	}
	return response, fmt.Errorf("oauth api error: status_code=%d body=%s", r.StatusCode(), r.Body())
}

func (client *oauthClient) RefreshOAuthToken(ctx context.Context, refreshToken string) (response OAuthResponse, err error) {
	r, err := client.client.R().
		SetBasicAuth(client.clientID, client.clientSecret).
		SetFormData(map[string]string{
			"refresh_token": refreshToken,
			"grant_type":    "refresh_token",
		}).
		SetResult(&response).
		SetContext(ctx).
		Post(oauthURL + "/token")
	if err != nil {
		return response, fmt.Errorf("oauth call error: %w", err)
	}
	if r.IsSuccess() {
		return response, nil
	}
	return response, fmt.Errorf("oauth api error: status_code=%d body=%s", r.StatusCode(), r.Body())
}
