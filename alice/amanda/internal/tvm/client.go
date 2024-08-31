package tvm

import (
	"context"

	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

type Client interface {
	GetServiceTicket() (string, error)
}

type client struct {
	destinationAlias string
	api              *tvmtool.Client
}

func (c *client) GetServiceTicket() (string, error) {
	return c.api.GetServiceTicketForAlias(context.Background(), c.destinationAlias)
}

func NewClient(destinationAlias string) (Client, error) {
	c, err := tvmtool.NewDeployClient()
	if err != nil {
		return nil, err
	}
	return &client{
		destinationAlias: destinationAlias,
		api:              c,
	}, nil
}
