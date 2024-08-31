package handlers

import (
	"context"

	"a.yandex-team.ru/alice/gamma/server/skills"
	api "a.yandex-team.ru/alice/gamma/server/webhook/api/admin"
)

type SkillAdmin struct {
	provider skills.Provider
}

func CreateAdmin(provider_ skills.Provider) *SkillAdmin {
	return &SkillAdmin{provider: provider_}
}

func (admin *SkillAdmin) GetSkill(ctx context.Context, skillID string) (*skills.Info, error) {
	return admin.provider.GetSkill(ctx, skillID)
}

func (admin *SkillAdmin) GetSkills(ctx context.Context, skillIDs []string) ([]*skills.Info, []error) {
	return admin.provider.GetSkills(ctx, skillIDs)
}

func (admin *SkillAdmin) GetTestSkill(ctx context.Context, requests []api.Request, urlBuilder func(string) string) []interface{} {
	skillIDs := api.ExtractSkillIds(requests)
	skillInfos, errors := admin.GetSkills(ctx, skillIDs)
	response := api.NewResponse(skillIDs, skillInfos, errors, urlBuilder)
	return response
}
