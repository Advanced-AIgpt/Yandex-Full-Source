package dialogs

import (
	"context"
)

type DialogsMock struct {
	AuthorizeSkillOwnerMock      func(context.Context, uint64, string, string) (AuthorizationData, error)
	GetSkillInfoMock             func(context.Context, string) (*SkillInfo, error)
	GetSmartHomeSkillsMock       func(context.Context, string) ([]SkillShortInfo, error)
	GetSkillCertifiedDevicesMock func(context.Context, string) (CertifiedDevices, error)
}

func (d *DialogsMock) AuthorizeSkillOwner(ctx context.Context, userID uint64, skillID, userTicket string) (AuthorizationData, error) {
	if d.AuthorizeSkillOwnerMock != nil {
		return d.AuthorizeSkillOwnerMock(ctx, userID, skillID, userTicket)
	}
	return AuthorizationData{}, nil
}

func (d *DialogsMock) GetSkillInfo(ctx context.Context, ticket string, s string) (*SkillInfo, error) {
	if d.GetSkillInfoMock != nil {
		return d.GetSkillInfoMock(ctx, ticket)
	}
	return nil, nil
}

func (d *DialogsMock) GetSmartHomeSkills(ctx context.Context, ticket string) ([]SkillShortInfo, error) {
	if d.GetSmartHomeSkillsMock != nil {
		return d.GetSmartHomeSkillsMock(ctx, ticket)
	}
	return nil, nil
}

func (d *DialogsMock) GetSkillCertifiedDevices(ctx context.Context, ticket string, s string) (CertifiedDevices, error) {
	if d.GetSkillCertifiedDevicesMock != nil {
		return d.GetSkillCertifiedDevicesMock(ctx, ticket)
	}
	return CertifiedDevices{}, nil
}
