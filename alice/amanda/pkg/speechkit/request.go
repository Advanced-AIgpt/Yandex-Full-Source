package speechkit

type Request struct {
	Header      *Header      `json:"header,omitempty"`
	Application *App         `json:"application,omitempty"`
	Body        *RequestBody `json:"request,omitempty"`
}

type RequestBody struct {
	Event             *Event                 `json:"event,omitempty"`
	Location          *Location              `json:"location,omitempty"`
	Experiments       map[string]interface{} `json:"experiments,omitempty"`
	DeviceState       *DeviceState           `json:"device_state,omitempty"`
	AdditionalOptions *AdditionalOptions     `json:"additional_options,omitempty"`
	VoiceSession      *bool                  `json:"voice_session,omitempty"`
	ResetSession      *bool                  `json:"reset_session,omitempty"`
	LAASRegion        *LAASRegion            `json:"laas_region,omitempty"`
}

type Location struct {
	Latitude  float64 `json:"lat"`
	Longitude float64 `json:"lon"`
}

type EventType string

const (
	TextInput    EventType = "text_input"
	VoiceInput   EventType = "voice_input"
	ServerAction EventType = "server_action"
	ImageInput   EventType = "image_input"
)

type Event struct {
	Type    EventType   `json:"type"`
	Text    string      `json:"text,omitempty"`
	Name    string      `json:"name,omitempty"`
	Payload interface{} `json:"payload,omitempty"`
}

type DeviceState map[string]interface{}

type BASSOptions struct {
	UserAgent         string   `json:"user_agent,omitempty"`
	FiltrationLevel   *uint32  `json:"filtration_level,omitempty"`
	ClientIP          string   `json:"client_ip,omitempty"`
	Cookies           string   `json:"cookies,omitempty"`
	ScreenScaleFactor *float64 `json:"screen_scale_factor,omitempty"`
	MegamindCGI       string   `json:"megamind_cgi_string,omitempty"`
	ProcessID         string   `json:"process_id,omitempty"`
	VideoGalleryLimit *int32   `json:"video_gallery_limit,omitempty"`
	RegionID          *int32   `json:"region_id,omitempty"`
}

type AdditionalOptions struct {
	OAuthToken          string       `json:"oauth_token,omitempty"`
	BASSOptions         *BASSOptions `json:"bass_options,omitempty"`
	SupportedFeatures   []string     `json:"supported_features,omitempty"`
	UnsupportedFeatures []string     `json:"unsupported_features,omitempty"`
}

type LAASRegion struct{}
