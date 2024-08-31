package rotor

import (
	"fmt"
	"time"

	"github.com/go-resty/resty/v2"
)

const (
	ViewportOnly ScreenshotMode = 0
	FullVisible  ScreenshotMode = 1
)

const (
	_rotorOnlineURL = "http://gorotor.zora.yandex.net:23555"
)

// ScreenshotMode is a parameter which determines the way of screenshot taking
type ScreenshotMode int

// Rotor is a web viewer
type Rotor struct {
	source string
	tvm    string
}

type ViewPortSize struct {
	Width  uint `json:"Width"`
	Height uint `json:"Height"`
}

type Options struct {
	EnableImages   bool           `json:"EnableImages"`
	ScreenshotMode ScreenshotMode `json:"ScreenshotMode"`
	ViewPortSize   ViewPortSize   `json:"ViewPortSize"`
}

type outputFormat struct {
	PNG  bool `json:"Png"`
	HTML bool `json:"Html"`
}

type viewOptions struct {
	Options
	OutputFormat outputFormat `json:"OutputFormat"`
}

type request struct {
	URL              string      `json:"Url"`
	Source           string      `json:"Source"`
	Options          viewOptions `json:"Options"`
	TVMServiceTicket string      `json:"TvmServiceTicket,omitempty"`
}

// NewRotor creates new Rotor Online viewer
func New(source, tvm string) *Rotor {
	return &Rotor{source, tvm}
}

func (rotor Rotor) ToPNG(url string) (image []byte, err error) {
	return rotor.ToPNGOpts(url, Options{
		EnableImages:   true,
		ScreenshotMode: FullVisible,
		ViewPortSize: ViewPortSize{
			Width:  350,
			Height: 250,
		},
	})
}

func (rotor Rotor) ToPNGOpts(url string, options Options) (image []byte, err error) {
	body := request{
		URL:    url,
		Source: rotor.source,
		Options: viewOptions{
			Options: options,
			OutputFormat: outputFormat{
				PNG:  true,
				HTML: false,
			},
		},
		TVMServiceTicket: rotor.tvm,
	}
	client := resty.New().SetTimeout(5 * time.Second).SetHostURL(_rotorOnlineURL)
	r, err := client.R().
		SetHeader("Content-Type", "application/json").
		SetBody(body).
		Post("/v1/rotor/execute/png")
	if err != nil {
		return nil, err
	}
	if r.IsError() {
		return nil, fmt.Errorf("server error status code %d: %s", r.StatusCode(), r.Body())
	}
	return r.Body(), nil
}
