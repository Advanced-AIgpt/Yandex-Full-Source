package skills

import (
	"context"
)

type Provider interface {
	GetSkill(ctx context.Context, skillID string) (*Info, error)
	GetSkills(ctx context.Context, skillIDs []string) ([]*Info, []error)
}

type Factory interface {
	CreateProvider() (Provider, error)
}
