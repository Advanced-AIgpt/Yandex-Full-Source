package avatars

import (
	"fmt"
	"time"

	"github.com/go-resty/resty/v2"
)

const (
	_url = "http://yandex.ru/images-apphost/image-download"
)

type Response struct {
	ImageID    string `json:"image_id"`
	URL        string `json:"url"`
	ImageShard int32  `json:"image_shard"`
	Height     int32  `json:"height"`
	Width      int32  `json:"width"`
	Namespace  string `json:"namespace"`
}

func Upload(image []byte) (response *Response, err error) {
	response = new(Response)
	client := resty.New().SetTimeout(5 * time.Second).SetRetryCount(2).SetHostURL(_url)
	r, err := client.R().
		SetBody(image).
		SetResult(response).
		Post("")
	if err != nil {
		return nil, err
	}
	if r.IsError() {
		return nil, fmt.Errorf("server error status code %d: %s", r.StatusCode(), r.Body())
	}
	return
}
