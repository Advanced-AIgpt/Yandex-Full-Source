package model

type RelativityType string

// small hack to get address of constant
func RT(r RelativityType) *RelativityType { return &r }
