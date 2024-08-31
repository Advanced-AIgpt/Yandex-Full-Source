package irhub

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type controller struct {
	logger log.Logger
	pf     provider.IProviderFactory
	db     db.DB
}

func NewController(logger log.Logger, pf provider.IProviderFactory, db db.DB) Controller {
	return &controller{logger, pf, db}
}

func (s *controller) IRHubRemotes(ctx context.Context, origin model.Origin, hub model.Device) (model.Devices, error) {
	ctxlog.Infof(ctx, s.logger, "fetching ir hub remotes for %s", hub.ID)
	providerClient, err := s.pf.NewProviderClient(ctx, origin, hub.SkillID)
	if err != nil {
		return nil, xerrors.Errorf("failed to create provider: %w", err)
	}
	tuyaProvider, ok := providerClient.(provider.TuyaBasedProvider)
	if !ok {
		return nil, xerrors.Errorf("skillID %s does no implement tuya based interface", err)
	}
	hubRemotesExternalIDs, err := tuyaProvider.GetHubRemotes(ctx, hub.ExternalID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get ir hub remotes: %w", err)
	}
	userDevices, err := s.db.SelectUserDevicesSimple(ctx, origin.User.ID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get devices: %w", err)
	}
	hubRemotes := userDevices.FilterBySkillID(hub.SkillID).FilterByExternalIDs(hubRemotesExternalIDs)
	return hubRemotes, nil
}
