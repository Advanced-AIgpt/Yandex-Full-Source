package main

import "context"

type ConfigDataSource struct {
	UsersID []string `yaml:"users_id"`
	SkillID string   `yaml:"skill_id"`
}

func (ds *ConfigDataSource) StreamUsersID(_ context.Context, usersCh chan string) {
	for _, userID := range ds.UsersID {
		usersCh <- userID
	}
}
