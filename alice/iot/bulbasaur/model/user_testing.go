package model

import (
	"math/rand"
)

type TestUser struct {
	User

	Rooms  map[string]Room
	Groups map[string]Group
}

func NewUser(login string) *TestUser {
	userID := uint64(rand.Uint32())<<32 + uint64(rand.Uint32())
	return &TestUser{
		User:   User{ID: userID, Login: login},
		Rooms:  make(map[string]Room),
		Groups: make(map[string]Group),
	}
}

func (user *TestUser) WithRooms(rooms ...string) *TestUser {
	for _, room := range rooms {
		user.Rooms[room] = Room{Name: room}
	}
	return user
}

func (user *TestUser) WithGroups(groups ...Group) *TestUser {
	for i := range groups {
		user.Groups[groups[i].Name] = groups[i]
	}
	return user
}
