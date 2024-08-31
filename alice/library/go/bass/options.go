package libbass

import (
	"time"

	"github.com/go-resty/resty/v2"
)

type RestyOption func(client *resty.Client) *resty.Client

var (
	DefaultRetryPolicyOption RestyOption = func(client *resty.Client) *resty.Client {
		client.GetClient().Timeout = time.Second * 3
		client.RetryCount = 5
		client.RetryWaitTime = 1 * time.Millisecond // do not try to pass 0 here, it's not working https://st.yandex-team.ru/IOT-851
		client.RetryMaxWaitTime = 10 * time.Millisecond
		return client
	}
)
