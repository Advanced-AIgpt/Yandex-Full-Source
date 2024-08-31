package server

import (
	"context"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) getIrHubRemotes(ctx context.Context, userID uint64, provider provider.TuyaBasedProvider, hub model.Device) ([]model.Device, error) {
	skillInfo := provider.GetSkillInfo()

	ctxlog.Infof(ctx, s.Logger, "fetching remotes for hub %s, skill_id %s", hub.ID, skillInfo.SkillID)
	remotesExtIDs, err := provider.GetHubRemotes(ctx, hub.ExternalID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get ir hub %s remotes from %s: %w", hub.ID, skillInfo.SkillID, err)
	}

	userDevices, err := s.db.SelectUserDevicesSimple(ctx, userID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get devices for user %d: %w", userID, err)
	}
	hubRemotes := make([]model.Device, 0, len(remotesExtIDs))
	for _, device := range userDevices {
		if tools.Contains(device.ExternalID, remotesExtIDs) && (device.SkillID == skillInfo.SkillID) {
			hubRemotes = append(hubRemotes, device)
		}
	}
	return hubRemotes, nil
}

func (s *Server) checkUserExists(ctx context.Context, userID uint64) error {
	if _, err := s.db.SelectUser(ctx, userID); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user: %v", err)
		if xerrors.Is(err, &model.UnknownUserError{}) {
			return &model.NoAddedDevicesError{}
		}
		return err
	}
	return nil
}
