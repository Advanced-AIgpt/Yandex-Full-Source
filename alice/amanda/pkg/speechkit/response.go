package speechkit

type Response struct {
	Header        *Header        `json:"header,omitempty"`
	Body          *ResponseBody  `json:"response,omitempty"`
	VoiceResponse *VoiceResponse `json:"voice_response,omitempty"`
}

type ResponseBody struct {
	Cards      []Card      `json:"cards"`
	Directives []Directive `json:"directives"`
	Suggests   *Suggests   `json:"suggest"`
}

type VoiceResponse struct {
	OutputSpeech *OutputSpeech `json:"output_speech,omitempty"`
	ShouldListen *bool         `json:"should_listen,omitempty"`
}

type OutputSpeech struct {
	Type string `json:"type"`
	Text string `json:"text"`
}

type Directive struct {
	Type          string                 `json:"type"`
	Name          string                 `json:"name"`
	AnalyticsType string                 `json:"sub_name,omitempty"`
	IgnoreAnswer  *bool                  `json:"ignore_answer,omitempty"`
	Payload       map[string]interface{} `json:"payload"`
}

type Theme struct {
	ImageURL string `json:"image_url"`
}

type Button struct {
	Type       string      `json:"type"`
	Title      string      `json:"title"`
	Directives []Directive `json:"directives"`
	Text       string      `json:"text"`
	Theme      *Theme      `json:"theme"`
}

type Card struct {
	Type    string                 `json:"type"`
	Text    string                 `json:"text"`
	Body    map[string]interface{} `json:"body"`
	Buttons []Button               `json:"buttons"`
}

type Suggests struct {
	Items []Button `json:"items"`
}
