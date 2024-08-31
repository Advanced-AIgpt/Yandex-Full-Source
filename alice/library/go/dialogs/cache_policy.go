package dialogs

type CachePolicy interface {
	UseCache(skillID string) bool
	SaveToCache(response dialogResponse, skillID string) bool
}

type cachePolicyHolder struct {
	useCacheFunc    func(skillID string) bool
	saveToCacheFunc func(response dialogResponse, skillID string) bool
}

func (c cachePolicyHolder) UseCache(skillID string) bool {
	if c.useCacheFunc == nil {
		return false
	}
	return c.useCacheFunc(skillID)
}

func (c cachePolicyHolder) SaveToCache(response dialogResponse, skillID string) bool {
	if c.saveToCacheFunc == nil {
		return false
	}
	return c.saveToCacheFunc(response, skillID)
}

var NoCachePolicy CachePolicy = cachePolicyHolder{
	useCacheFunc: func(skillID string) bool {
		return false
	},
	saveToCacheFunc: func(response dialogResponse, skillID string) bool {
		return false
	},
}

var BulbasaurCachePolicy CachePolicy = cachePolicyHolder{
	useCacheFunc: func(skillID string) bool {
		return !isDraft(skillID)
	},
	saveToCacheFunc: func(response dialogResponse, skillID string) bool {
		return !response.AccessToSkillTesting.HasAccess && !isDraft(skillID)
	},
}

var SteelixCachePolicy CachePolicy = cachePolicyHolder{
	useCacheFunc: func(skillID string) bool {
		return !isDraft(skillID)
	},
	saveToCacheFunc: func(response dialogResponse, skillID string) bool {
		return !isDraft(skillID)
	},
}
