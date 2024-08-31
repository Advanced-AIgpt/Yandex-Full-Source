package staff

import (
	"fmt"
	"strings"
	"time"

	"github.com/go-resty/resty/v2"
)

const (
	_baseURL    = "https://staff-api.yandex-team.ru/v3"
	_personsURL = _baseURL + "/persons"
)

type User struct {
	Login    string        `json:"login"`
	Official *OfficialInfo `json:"official,omitempty"`
}

type OfficialInfo struct {
	IsDismissed bool `json:"is_dismissed"`
}

type API struct {
	token string
}

func (api *API) GetUserByTelegramUsername(username string) (user *User, err error) {
	user = new(User)
	client := resty.New().SetTimeout(5 * time.Second).SetRetryCount(2)
	r, err := client.R().
		SetHeader("Authorization", fmt.Sprintf("OAuth %s", api.token)).
		SetQueryParam("_fields", "login,official.is_dismissed").
		SetQueryParam("_one", "1").
		SetQueryParam("_query", makeStaffQuery(username)).
		SetResult(user).
		Get(_personsURL)
	if err != nil {
		return nil, err
	}
	code := r.StatusCode()
	if code != 200 {
		return nil, fmt.Errorf("status code: %d", code)
	}
	return user, nil
}

func New(token string) API {
	return API{token: token}
}

func makeStaffQuery(username string) string {
	match := fmt.Sprintf(`{"type": "telegram", "value_lower": "%s"}`, strings.ToLower(username))
	return fmt.Sprintf("accounts == match(%s) and official.is_dismissed == False", match)
}
