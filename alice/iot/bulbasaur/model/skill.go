package model

type SkillID string

type DiscoveryMethod string

var (
	AccountLinkingDiscoveryMethod DiscoveryMethod = "account_linking"
	ZigbeeDiscoveryMethod         DiscoveryMethod = "zigbee"
)

func (s SkillID) GetDiscoveryMethods() []DiscoveryMethod {
	connectionMethods := []DiscoveryMethod{AccountLinkingDiscoveryMethod}
	if ZigbeeSkills[s] {
		connectionMethods = append(connectionMethods, ZigbeeDiscoveryMethod)
	}
	return connectionMethods
}

var ZigbeeSkills = map[SkillID]bool{
	SkillID(XiaomiSkill): true,
	SkillID(AqaraSkill):  true,
}
