package skills

import "golang.org/x/xerrors"

var NotFoundError = xerrors.New("skill not found")

type Info struct {
	ID   string `yaml:"id" json:"id"`
	Addr string `yaml:"addr" json:"addr"`
}
