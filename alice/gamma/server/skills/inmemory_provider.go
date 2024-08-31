package skills

import (
	"context"
	"sync"
)

type InMemorySkillProvider struct {
	skillsMap map[string]Info
	mutex     *sync.Mutex
}

func (provider *InMemorySkillProvider) getSkill(ctx context.Context, skillID string) (*Info, error) {
	if skill_, ok := provider.skillsMap[skillID]; ok {
		return &skill_, nil
	}
	return nil, NotFoundError
}

func (provider *InMemorySkillProvider) GetSkill(ctx context.Context, skillID string) (*Info, error) {
	provider.mutex.Lock()
	defer provider.mutex.Unlock()
	return provider.getSkill(ctx, skillID)
}

func (provider *InMemorySkillProvider) GetSkills(ctx context.Context, skillIDs []string) ([]*Info, []error) {
	provider.mutex.Lock()
	defer provider.mutex.Unlock()

	skills := make([]*Info, len(skillIDs))
	errors := make([]error, len(skillIDs))
	for i, skillID := range skillIDs {
		skills[i], errors[i] = provider.getSkill(ctx, skillID)
	}
	return skills, errors
}

type InMemoryProviderFactory struct {
	SkillsMap map[string]Info
}

func (factory *InMemoryProviderFactory) CreateProvider() (provider Provider, err error) {
	return &InMemorySkillProvider{
		factory.SkillsMap,
		&sync.Mutex{},
	}, nil
}
