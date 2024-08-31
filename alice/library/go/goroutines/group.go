package goroutines

import (
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Group addition handle panic to golang.org/x/sync/errgroup
type Group struct {
	errgroup.Group
}

func (group *Group) Go(f func() error) {
	group.Group.Go(func() (err error) {
		defer func() {
			if r := recover(); r != nil {
				err = xerrors.Errorf("caught panic: %v", r)
			}
		}()

		err = f()
		return err
	})
}
