package xiva

import (
	"fmt"
	"time"

	"github.com/go-resty/resty/v2"
)

const (
	_bassURL = "http://bass-prod.yandex.net"
)

type Service interface {
	PlayText(text, deviceID, userID string) error
}

func New() Service {
	return &service{}
}

type service struct{}

func (s *service) PlayText(text, deviceID, userID string) error {
	client := resty.New().SetTimeout(1 * time.Second).SetHostURL(_bassURL)
	r, err := client.R().
		SetHeader("Content-Type", "application/json").
		SetBody(map[string]interface{}{
			"callback_data": fmt.Sprintf("{\"uid\": \"%s\",\"did\": \"%s\",\"client_id\": \"ru.yandex.quasar.iot\"}", userID, deviceID),
			"event":         "phrase_action",
			"service":       "quasar",
			"service_data": map[string]string{
				"phrase": text,
			},
		}).
		Post("/push")
	if err != nil {
		return fmt.Errorf("xiva error: %w", err)
	}
	if r.IsError() {
		return fmt.Errorf("xiva error: %s", r.Error())
	}
	return nil
}
