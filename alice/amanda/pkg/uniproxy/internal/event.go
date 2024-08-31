package internal

type SynchronizeState struct {
	AuthToken   string `json:"auth_token"`
	UUID        string `json:"uuid"`
	Lang        string `json:"lang"`
	Voice       string `json:"voice"`
	VINSPartial bool   `json:"vins_partial"`

	OAuthToken              *string `json:"oauth_token,omitempty"`
	SpeechKitVersion        *string `json:"speechkitVersion,omitempty"`
	Device                  *string `json:"device,omitempty"`
	PlatformInfo            *string `json:"platform_info,omitempty"`
	NetworkType             *string `json:"network_type,omitempty"` // wifi,2g,3g
	SaveToMDS               *bool   `json:"save_to_mds,omitempty"`  // true by default
	DisableFallback         *bool   `json:"disable_fallback,omitempty"`
	DisableLocalExperiments *bool   `json:"disable_local_experiments,omitempty"`
	VINSURL                 string  `json:"vinsUrl,omitempty"`
}
