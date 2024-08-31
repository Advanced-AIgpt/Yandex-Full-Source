package socialism

import "context"

type IClient interface {
	GetUserApplicationToken(ctx context.Context, uid uint64, skillInfo SkillInfo) (string, error)
	DeleteUserToken(ctx context.Context, uid uint64, skillInfo SkillInfo) error
	CheckUserAppTokenExists(ctx context.Context, uid uint64, skillInfo SkillInfo) (bool, error)
}
