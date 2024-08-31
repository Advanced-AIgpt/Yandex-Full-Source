package sup

import (
	"fmt"
	"net/url"
)

// https://doc.yandex-team.ru/sup-api/concepts/push-create
type PushRequest struct {
	Receivers            []IReceiver       `json:"receiver"`
	TTL                  uint64            `json:"ttl"` // seconds, range: 0..2419000
	Notification         PushNotification  `json:"notification"`
	Project              string            `json:"project"`
	Schedule             string            `json:"schedule,omitempty"`
	AdjustTimezone       bool              `json:"adjust_time_zone,omitempty"`
	SendTimeFrame        string            `json:"send_time_frame,omitempty"`
	Data                 PushData          `json:"data,omitempty"`
	SpreadInterval       uint64            `json:"spread_interval,omitempty"` // for flattenning the rps curve
	MaxExpectedReceivers uint64            `json:"max_expected_receivers,omitempty"`
	ThrottlePolicies     *ThrottlePolicies `json:"throttle_policies,omitempty"`
	Repacks              Repacks           `json:"repack,omitempty"` // for overriding deeplinks to apps
}

type PushActionType string

type PushData struct {
	PushID    string `json:"push_id,omitempty"`
	ContentID string `json:"content_id,omitempty"`
	StatID    string `json:"stat_id,omitempty"`
	Tag       string `json:"tag,omitempty"`

	// similar to PushNotification link field
	PushURI    string         `json:"push_uri,omitempty"`
	PushAction PushActionType `json:"push_action,omitempty"`
}

type PushNotification struct {
	Title  string `json:"title,omitempty"`
	Body   string `json:"body,omitempty"` // push notification text
	Icon   string `json:"icon,omitempty"` // link to push icon
	IconID string `json:"iconId,omitempty"`
	Link   string `json:"link,omitempty"`
}

type PushResponse struct {
	ID        string `json:"id"`
	Receivers uint64 `json:"recievers,omitempty"`
}

type ThrottlePolicies struct {
	InstallID string `json:"install_id,omitempty"`
	DeviceID  string `json:"device_id,omitempty"`
}

type Repacks []Repack

type Repack struct {
	App  RepackApp  `json:"app"`
	Push RepackPush `json:"push"`
}

type RepackApp struct {
	Platform string   `json:"platform"`
	Apps     []string `json:"apps"`
	Version  string   `json:"version"`
}

type RepackPush struct {
	Notification PushNotification `json:"notification"`
	Data         *PushData        `json:"data,omitempty"` // IOT-1301: android could not use repack properly without it
}

func IoTIOSLinkWrapper(link string) string {
	return fmt.Sprintf("yandexiot://?url=%s", url.QueryEscape(link))
}

func IoTAndroidLinkWrapper(link string) string {
	return fmt.Sprintf("yandexiot://?uri=%s", url.QueryEscape(link))
}

func SearchAppLinkWrapper(link string) string {
	return fmt.Sprintf("ya-search-app-open://?uri=%s", url.QueryEscape(link))
}

func QuasarLinkWrapper(link string) string {
	return fmt.Sprintf("yellowskin://?url=%s", url.QueryEscape(link))
}
