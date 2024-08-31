package session

import (
	"time"

	"go.mongodb.org/mongo-driver/bson/primitive"
)

type Session struct {
	ID        primitive.ObjectID `bson:"_id" json:"-"`
	ChatID    int64              `bson:"chat_id" json:"chat_id"`
	Settings  Settings           `bson:"settings" json:"settings"`
	State     *string            `bson:"state" json:"state"`
	Dynamic   Dynamic            `bson:"dynamic" json:"dynamic"`
	Analytics Analytics          `bson:"analytics" json:"analytics"`
	Reset     bool               `bson:"reset" json:"reset"`
}

type sessionView struct {
	ChatID    int64     `bson:"chat_id" json:"chat_id"`
	Settings  Settings  `bson:"settings" json:"settings"`
	State     *string   `bson:"state" json:"state"`
	Dynamic   Dynamic   `bson:"dynamic" json:"dynamic"`
	Analytics Analytics `bson:"analytics" json:"analytics"`
	Reset     bool      `bson:"reset" json:"reset"`
}

func newSessionView(s *Session) *sessionView {
	return &sessionView{
		ChatID:    s.ChatID,
		Settings:  s.Settings,
		State:     s.State,
		Dynamic:   s.Dynamic,
		Analytics: s.Analytics,
		Reset:     s.Reset,
	}
}

type Analytics struct {
	RegistrationTime time.Time `bson:"registration_time" json:"registration_time"`
	LastRequestTime  time.Time `bson:"last_request_time" json:"last_request_time"`
}

type AuthData struct {
	VerificationTime time.Time `bson:"verification_time" json:"verification_time"`
	IsYandexoid      bool      `bson:"is_yandexoid" json:"is_yandexoid"`
}

type Dynamic struct {
	Buttons    []Button    `bson:"buttons" json:"buttons"`
	Directives []Directive `bson:"directives" json:"directives"`
	Suggests   []Button    `bson:"suggests" json:"suggests"`
	AuthData   *AuthData   `bson:"auth_data" json:"auth_data"`
}

type Button struct {
	Key  string    `bson:"key" json:"key"`
	Data string    `bson:"data" json:"data"`
	Time time.Time `bson:"time" json:"time"`
}

type Directive struct {
	Key  string    `bson:"key" json:"key"`
	Data string    `bson:"data" json:"data"`
	Time time.Time `bson:"time" json:"time"`
}

type AccountInfo struct {
	Username   string `bson:"username" json:"username"`
	OAuthToken string `bson:"oauth_token" json:"-"`
}

type AccountDetails struct {
	Accounts map[string]AccountInfo `bson:"accounts" json:"accounts"`
	Active   *string                `bson:"active" json:"active"`
}

type ApplicationDetails struct {
	AppID              string `bson:"app_id" json:"app_id"`
	AppVersion         string `bson:"app_version" json:"app_version"`
	OSVersion          string `bson:"os_version" json:"os_version"`
	Platform           string `bson:"platform" json:"platform"`
	Language           string `bson:"lang" json:"lang"`
	DeviceID           string `bson:"device_id" json:"device_id"`
	DeviceModel        string `bson:"device_model" json:"device_model"`
	DeviceManufacturer string `bson:"device_manufacturer" json:"device_manufacturer"`
	UUID               string `bson:"uuid" json:"uuid"`
	UserAgent          string `bson:"user_agent" json:"user_agent"`
	DeviceColor        string `bson:"device_color" json:"device_color"`
}

type Artifact struct {
	RequestID string
	Intent    string
	Time      time.Time
}

type Device struct {
	ID       string `bson:"id" json:"id"`
	UserID   string `bson:"user_id" json:"user_id"`
	Name     string `bson:"name" json:"name"`
	IsActive bool   `bson:"is_active" json:"is_active"`
}

type SystemDetails struct {
	UniproxyURL                string     `bson:"uniproxy_url" json:"uniproxy_url"`
	UniproxyAuthToken          string     `bson:"uniproxy_auth_token" json:"-"`
	VINSURL                    string     `bson:"vinsurl" json:"vinsurl"`
	VINSURLHistory             []string   `bson:"vinsurl_history" json:"vinsurl_history"`
	UniproxyURLHistory         []string   `bson:"uniproxy_url_history" json:"uniproxy_url_history"`
	ASRTopic                   string     `bson:"asr_topic" json:"asr_topic"`
	History                    []Artifact `bson:"history" json:"history"`
	Voice                      string     `bson:"voice" json:"voice"`
	VoiceSession               *bool      `bson:"voice_session" json:"voice_session"`
	DeferredDirectiveExecution bool       `bson:"deferred_directive_execution" json:"deferred_directive_execution"`
	ResetSession               bool       `bson:"reset_session" json:"reset_session"`
	HideHinds                  bool       `bson:"hide_hinds" json:"hide_hinds"`
	TextualTTS                 bool       `bson:"textual_tts" json:"textual_tts"`
	ConnectedDevices           []Device   `bson:"connected_devices" json:"connected_devices"`
	PlayTTSOnActiveDevice      bool       `bson:"play_tts_on_active_device" json:"play_tts_on_active_device"`
	DontSendTTSAsVoiceMessage  bool       `bson:"dont_send_tts_as_voice_message" json:"dont_send_tts_as_voice_message"`
}

type Experiment struct {
	NumValue    *float64 `bson:"num_value" json:"num_value"`
	StringValue *string  `bson:"string_value" json:"string_value"`
	Disabled    bool     `bson:"disabled" json:"disabled"`
}

type Location struct {
	Latitude  float64 `bson:"latitude" json:"latitude"`
	Longitude float64 `bson:"longitude" json:"longitude"`
}

type QueryParam struct {
	Disabled bool `bson:"disabled" json:"disabled"`
}

type Settings struct {
	AccountDetails     AccountDetails        `bson:"account_details" json:"account_details"`
	ApplicationDetails ApplicationDetails    `bson:"application_details" json:"application_details"`
	SystemDetails      SystemDetails         `bson:"system_details" json:"system_details"`
	Experiments        map[string]Experiment `bson:"experiments" json:"experiments"`
	Location           Location              `bson:"location" json:"location"`
	SupportedFeatures  map[string]bool       `bson:"supported_features" json:"supported_features"`
	DeviceState        string                `bson:"device_state" json:"device_state"`
	RegionID           *int32                `bson:"region_id" json:"region_id"`
	QueryParams        map[string]QueryParam `bson:"query_params" json:"query_params"`
}

func New(chatID int64) *Session {
	return &Session{
		ChatID: chatID,
	}
}
