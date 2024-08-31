package main

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
	"context"
	"path"
	"strings"
)

func cleanupDatabase(ctx context.Context, client *db.DBClient) error {
	s := scheme.Client{
		Driver: *client.Driver,
	}
	var list func(int, string) error
	list = func(i int, p string) error {
		dir, err := s.ListDirectory(ctx, p)
		if err != nil {
			return xerrors.Errorf("cannot ListDirectory: %w", err)
		}
		logger.Infof(strings.Repeat(" ", i*2), "inspecting ", dir.Name, dir.Type)
		for _, c := range dir.Children {
			pt := path.Join(p, c.Name)
			switch c.Type {
			case scheme.EntryDirectory:
				if err := list(i+1, pt); err != nil {
					return err
				}
				logger.Infof(strings.Repeat(" ", i*2), "removing ", c.Type, pt)
				if err := s.RemoveDirectory(ctx, pt); err != nil {
					return err
				}

			case scheme.EntryTable:
				s, err := client.SessionPool.Get(ctx)
				if err != nil {
					return err
				}
				defer func() { _ = client.SessionPool.Put(ctx, s) }()
				logger.Infof(strings.Repeat(" ", i*2), "dropping ", c.Type, pt)
				if err := s.DropTable(ctx, pt); err != nil {
					return err
				}

			default:
				logger.Infof(strings.Repeat(" ", i*2), "skipping ", c.Type, pt)
			}
		}
		return nil
	}

	return list(0, client.Prefix)
}

func backupOldTables(ctx context.Context, sp *table.SessionPool, prefix string) (err error) {
	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "DeviceGroups_old"), path.Join(prefix, "DeviceGroups"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Devices_old"), path.Join(prefix, "Devices"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Groups_old"), path.Join(prefix, "Groups"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Rooms_old"), path.Join(prefix, "Rooms"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Scenarios_old"), path.Join(prefix, "Scenarios"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "UserDevices_old"), path.Join(prefix, "UserDevices"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "UserScenarios_old"), path.Join(prefix, "UserScenarios"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Users_old"), path.Join(prefix, "Users"))
		}),
	)
	if err != nil {
		return err
	}

	return nil
}

func copyHcopy(ctx context.Context, sp *table.SessionPool, prefix string) (err error) {
	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "DeviceGroups"), path.Join(prefix, "DeviceGroups_hcopy"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Devices"), path.Join(prefix, "Devices_hcopy"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Groups"), path.Join(prefix, "Groups_hcopy"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Rooms"), path.Join(prefix, "Rooms_hcopy"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Scenarios"), path.Join(prefix, "Scenarios_hcopy"))
		}),
	)
	if err != nil {
		return err
	}

	err = table.Retry(ctx, sp,
		table.OperationFunc(func(ctx context.Context, s *table.Session) error {
			return s.CopyTable(ctx, path.Join(prefix, "Users"), path.Join(prefix, "Users_hcopy"))
		}),
	)
	if err != nil {
		return err
	}

	return nil
}
