package iot

import (
	"context"
	"errors"
	"fmt"
	"strings"

	"github.com/go-resty/resty/v2"
)

const (
	baseURL = "http://iot.quasar.yandex.net"
)

type Client interface {
	GetUserInfo(ctx context.Context) (UserInfo, error)
	RunScenario(ctx context.Context, scenarioID string) error
}

type iotClient struct {
	token  string
	client *resty.Client
}

func NewClient(token string, client *resty.Client) Client {
	return &iotClient{
		token:  token,
		client: client,
	}
}

type (
	UserInfo struct {
		Rooms     Rooms     `json:"rooms"`
		Groups    Groups    `json:"groups"`
		Devices   Devices   `json:"devices"`
		Scenarios Scenarios `json:"scenarios"`
	}
	Rooms []Room
	Room  struct {
		ID      string   `json:"id"`
		Name    string   `json:"name"`
		Devices []string `json:"devices"`
	}
	Groups []Group
	Group  struct {
		ID      string   `json:"id"`
		Name    string   `json:"name"`
		Type    string   `json:"type"`
		Devices []string `json:"devices"`
	}
	Devices []Device
	Device  struct {
		ID         string   `json:"id"`
		Name       string   `json:"name"`
		Type       string   `json:"type"`
		Room       string   `json:"room"`
		Groups     []string `json:"groups"`
		ExternalID string   `json:"external_id"`
	}
	Scenarios []Scenario
	Scenario  struct {
		ID       string `json:"id"`
		Name     string `json:"name"`
		IsActive bool   `json:"is_active"`
	}
)

func (d *Device) GetDeviceID() string {
	return strings.SplitN(d.ExternalID, ".", 2)[0]
}

func (r Rooms) Find(id string) (Room, error) {
	for _, room := range r {
		if room.ID == id {
			return room, nil
		}
	}
	return Room{}, errors.New("not found")
}

func (c *iotClient) GetUserInfo(ctx context.Context) (info UserInfo, err error) {
	r, err := c.client.R().
		SetHeader("Authorization", fmt.Sprintf("OAuth %s", c.token)).
		SetContext(ctx).
		SetResult(&info).
		Get(baseURL + "/api/v1.0/user/info")
	if err != nil {
		return UserInfo{}, fmt.Errorf("bulbasaur call error: %w", err)
	}
	code := r.StatusCode()
	if code != 200 {
		return UserInfo{}, fmt.Errorf("bulbasaur api error: status_code=%d body=%s", code, r.Body())
	}
	return
}

func (c *iotClient) RunScenario(ctx context.Context, scenarioID string) error {
	r, err := c.client.R().
		SetHeader("Authorization", fmt.Sprintf("OAuth %s", c.token)).
		SetContext(ctx).
		Post(fmt.Sprintf("%s/api/v1.0/scenarios/%s/actions", baseURL, scenarioID))
	if err != nil {
		return fmt.Errorf("bulbasaur call error: %w", err)
	}
	code := r.StatusCode()
	if code != 200 {
		return fmt.Errorf("bulbasaur api error: status_code=%d body=%s", code, r.Body())
	}
	return nil
}
