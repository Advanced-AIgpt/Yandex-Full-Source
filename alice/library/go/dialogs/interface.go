package dialogs

import (
	"context"
)

type Dialoger interface {
	AuthorizeSkillOwner(ctx context.Context, userID uint64, skillID, userTicket string) (AuthorizationData, error)
	GetSkillInfo(ctx context.Context, skillID, ticket string) (*SkillInfo, error)
	GetSmartHomeSkills(ctx context.Context, ticket string) ([]SkillShortInfo, error)
	GetSkillCertifiedDevices(ctx context.Context, skillID string, ticket string) (CertifiedDevices, error)
}
