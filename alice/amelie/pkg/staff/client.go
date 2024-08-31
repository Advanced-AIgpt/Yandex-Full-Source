package staff

import (
	"context"
	"fmt"
	"strings"

	"github.com/go-resty/resty/v2"
)

const (
	baseURL    = "https://staff-api.yandex-team.ru/v3"
	personsURL = baseURL + "/persons"
)

type User struct {
	Login    string        `json:"login"`
	Official *OfficialInfo `json:"official,omitempty"`
}

type OfficialInfo struct {
	IsDismissed bool `json:"is_dismissed"`
}

type Client interface {
	GetUserByTelegramUsername(ctx context.Context, username string) (user User, err error)
}

func NewClient(token string, client *resty.Client) Client {
	return &staffClient{
		token:  token,
		client: client,
	}
}

type staffClient struct {
	token  string
	client *resty.Client
}

func (staff *staffClient) GetUserByTelegramUsername(ctx context.Context, username string) (user User, err error) {
	r, err := staff.client.R().
		SetHeader("Authorization", fmt.Sprintf("OAuth %s", staff.token)).
		SetContext(ctx).
		SetQueryParam("_one", "1").
		SetQueryParam("_fields", "login,official.is_dismissed").
		SetQueryParam("_query", makeStaffQuery(username)).
		SetResult(&user).
		Get(personsURL)
	if err != nil {
		return User{}, fmt.Errorf("staff call error: %w", err)
	}
	code := r.StatusCode()
	if code != 200 {
		return User{}, fmt.Errorf("staff api error: status_code=%d body=%s", code, r.Body())
	}
	return user, nil
}

func makeStaffQuery(username string) string {
	match := fmt.Sprintf(`{"type": "telegram", "value_lower": "%s"}`, strings.ToLower(username))
	return fmt.Sprintf("accounts == match(%s) and official.is_dismissed == False", match)
}
