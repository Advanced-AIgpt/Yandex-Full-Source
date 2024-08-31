package sup

import (
	"net/url"

	"a.yandex-team.ru/library/go/core/log"
)

type Option func(*Client) error

func WithToken(token string) Option {
	return func(c *Client) error {
		c.token = token
		return nil
	}
}

func WithHost(host string) Option {
	return func(c *Client) error {
		parsedHost, err := url.Parse(host)
		if err != nil {
			return err
		}
		c.host = parsedHost.Host
		c.httpClient.SetHostURL(host)
		return nil
	}
}

func WithLogger(logger log.Logger) Option {
	return func(c *Client) error {
		c.Logger = logger
		return nil
	}
}
