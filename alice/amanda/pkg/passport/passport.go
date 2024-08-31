package passport

import (
	"fmt"
	"time"

	"github.com/go-resty/resty/v2"
)

const (
	_oAuthURL = "https://oauth.yandex.ru"
	_loginURL = "https://login.yandex.ru"
)

type API struct {
	clientID     string
	clientSecret string
}

func New(clientID, clientSecret string) *API {
	return &API{clientID: clientID, clientSecret: clientSecret}
}

func (api *API) GenerateCodeURL() string {
	return fmt.Sprintf("%s/authorize?response_type=code&client_id=%s&force_confirm=yes", _oAuthURL, api.clientID)
}

type OAuthResponse struct {
	TokenType    string `json:"token_type"`
	AccessToken  string `json:"access_token"`
	ExpiresIn    int64  `json:"expires_in"`
	RefreshToken string `json:"refresh_token"`
}

type OAuthError struct {
	Code        string `json:"error"`
	Description string `json:"error_description"`
}

func (api *API) GetOAuthToken(code string) (response *OAuthResponse, err error) {
	url := _oAuthURL + "/token"
	response = &OAuthResponse{}
	errorResponse := &OAuthError{}
	client := resty.New().SetTimeout(5 * time.Second).SetRetryCount(2)
	r, err := client.R().
		SetBasicAuth(api.clientID, api.clientSecret).
		SetFormData(map[string]string{
			"code":       code,
			"grant_type": "authorization_code",
		}).
		SetResult(response).
		SetError(errorResponse).
		Post(url)
	if err != nil {
		return nil, fmt.Errorf("unable to make request: %w", err)
	}
	if r.IsSuccess() {
		return response, nil
	}
	return nil, fmt.Errorf("request failed: %#v", errorResponse)
}

type BaseResponse struct {
	Username         string   `json:"login"`
	UserID           string   `json:"id"`
	ClientID         string   `json:"client_id"`
	OpenIDIdentities []string `json:"openid_identities"`
}

type EmailResponse struct {
	DefaultEmail string   `json:"default_email"`
	Emails       []string `json:"emails"`
}

type PictureResponse struct {
	IsAvatarEmpty   bool   `json:"is_avatar_empty"`
	DefaultAvatarID string `json:"default_avatar_id"`
}

type BirthdayResponse struct {
	Birthday string `json:"birthday"`
}

type UserResponse struct {
	FirstName   string `json:"first_name"`
	LastName    string `json:"last_name"`
	DisplayName string `json:"display_name"`
	RealName    string `json:"real_name"`
	Sex         string `json:"sex"`
}

// See: https://tech.yandex.com/passport/doc/dg/reference/response-docpage/
type UserInfoResponse struct {
	*BaseResponse
	*EmailResponse
	*PictureResponse
	*BirthdayResponse
	*UserResponse
}

func (api *API) GetUserInfo(accessToken string) (response *UserInfoResponse, err error) {
	url := _loginURL + "/info"
	response = &UserInfoResponse{}
	client := resty.New().SetTimeout(5 * time.Second).SetRetryCount(2)
	r, err := client.R().
		SetHeader("Authorization", "OAuth "+accessToken).
		SetQueryParam("format", "json").
		SetResult(response).
		Get(url)
	if err != nil {
		return nil, fmt.Errorf("unable to make request: %w", err)
	}
	if r.IsError() {
		return nil, fmt.Errorf("request failed: %s", r.Status())
	}
	return response, nil
}
