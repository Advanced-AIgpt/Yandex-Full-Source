package authpolicy

import (
	"github.com/go-resty/resty/v2"
)

type HTTPPolicy interface {
	Apply(*resty.Request) error
}
