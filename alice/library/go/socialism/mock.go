package socialism

import (
	"context"
)

type Mock struct {
	GetUserApplicationTokenMock func(ctx context.Context, uid uint64, skillInfo SkillInfo) (string, error)
	DeleteUserTokenMock         func(ctx context.Context, uid uint64, skillInfo SkillInfo) error
	CheckUserAppTokenExistsMock func(ctx context.Context, uid uint64, skillInfo SkillInfo) (bool, error)
}

func (s *Mock) GetUserApplicationToken(ctx context.Context, uid uint64, skillInfo SkillInfo) (string, error) {
	if s.CheckUserAppTokenExistsMock != nil {
		return s.GetUserApplicationTokenMock(ctx, uid, skillInfo)
	}
	return "", nil
}

func (s *Mock) DeleteUserToken(ctx context.Context, uid uint64, skillInfo SkillInfo) error {
	if s.DeleteUserTokenMock != nil {
		return s.DeleteUserTokenMock(ctx, uid, skillInfo)
	}
	return nil
}

func (s *Mock) CheckUserAppTokenExists(ctx context.Context, uid uint64, skillInfo SkillInfo) (bool, error) {
	if s.CheckUserAppTokenExistsMock != nil {
		return s.CheckUserAppTokenExistsMock(ctx, uid, skillInfo)
	}
	return false, nil
}
