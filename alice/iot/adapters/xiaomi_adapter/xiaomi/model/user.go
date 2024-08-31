package model

type ExternalUser struct {
	ExternalUserID  string
	UserIDs         []uint64
	SubscriptionKey string
}
