//go:build !cgo
// +build !cgo

package inflector

import (
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	Logger log.Logger
}

func (i *Client) Inflect(text string, grams []string) (Inflection, error) {
	return Inflection{}, xerrors.New("inflector not available without cgo")
}

func (i *Client) InflectWithHints(tokens []string, hints []string, grams []string) (Inflection, error) {
	return Inflection{}, xerrors.New("inflector not available without cgo")
}
