package passport

import (
	"context"
	"fmt"

	"github.com/go-resty/resty/v2"
)

const (
	loginURL = "https://login.yandex.ru"
)

type Client interface {
	GetUserInfo(ctx context.Context, accessToken string) (response UserInfoResponse, err error)
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

func NewPassportClient(client *resty.Client) Client {
	return &passportClient{
		client: client,
	}
}

type passportClient struct {
	client *resty.Client
}

func (client *passportClient) GetUserInfo(ctx context.Context, accessToken string) (response UserInfoResponse, err error) {
	r, err := client.client.R().
		SetHeader("Authorization", "OAuth "+accessToken).
		SetQueryParam("format", "json").
		SetResult(&response).
		SetContext(ctx).
		Get(loginURL + "/info")
	if err != nil {
		return response, fmt.Errorf("passport call error: %w", err)
	}
	if r.IsError() {
		return response, fmt.Errorf("passport api error: status_code=%d body=%s", r.StatusCode(), r.Body())
	}
	return response, nil
}
