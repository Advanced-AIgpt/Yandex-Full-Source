package model

import (
	"context"

	"a.yandex-team.ru/alice/library/go/userctx"
)

type User struct {
	ID     uint64
	Login  string
	Ticket string `json:"-"`
	IP     string `json:"-"`
}

func (u User) GetID() uint64 {
	return u.ID
}

func (u User) GetTicket() string {
	return u.Ticket
}

func (u User) Clone() User {
	return User{
		u.ID,
		u.Login,
		u.Ticket,
		u.IP,
	}
}

func (u *User) IsEmpty() bool {
	return *u == User{}
}

func (u *User) ToUserctxUser() userctx.User {
	return userctx.User{
		ID:     u.ID,
		Login:  u.Login,
		Ticket: u.Ticket,
		IP:     u.IP,
	}
}

func GetUserFromContext(ctx context.Context) (User, error) {
	bbUser, err := userctx.GetUser(ctx)
	if err != nil {
		return User{}, err
	}
	return User{
		ID:     bbUser.ID,
		Login:  bbUser.Login,
		Ticket: bbUser.Ticket,
		IP:     bbUser.IP,
	}, nil
}

type Users []User

func (users Users) IDs() []uint64 {
	result := make([]uint64, 0, len(users))
	for _, user := range users {
		result = append(result, user.ID)
	}
	return result
}

type ExtendedUserInfo struct {
	User     User
	UserInfo UserInfo
}
