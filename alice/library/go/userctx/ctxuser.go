package userctx

import (
	"context"
	"fmt"
	"strconv"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type ctxKey int

const (
	userKey ctxKey = iota
	oauthKey
	authErrKey
)

type User struct {
	ID     uint64
	Login  string
	Ticket string `json:"-"`
	IP     string
}

func (u User) GetID() uint64 {
	return u.ID
}

func (u User) GetTicket() string {
	return u.Ticket
}

func (u User) GetIP() string {
	return u.IP
}

func (u User) Clone() User {
	return User{
		ID:     u.ID,
		Login:  u.Login,
		Ticket: u.Ticket,
		IP:     u.IP,
	}
}

type OAuth struct {
	ClientID string
	Scope    []string
}

func WithUser(ctx context.Context, user User) context.Context {
	if ctx == nil {
		panic("cannot create context from nil parent")
	}
	storedUser, ok := ctx.Value(userKey).(*User)
	if !ok {
		storedUser = &User{}
		ctx = context.WithValue(ctx, userKey, storedUser)
		ctx = ctxlog.WithFields(ctx, log.String("user_id", strconv.FormatUint(user.ID, 10)))
	}
	*storedUser = user.Clone()
	return ctx
}

func WithAuthErr(ctx context.Context, authErr error) context.Context {
	ctx = context.WithValue(ctx, authErrKey, authErr)
	return ctx
}

func WithOAuth(ctx context.Context, oauth OAuth) context.Context {
	return context.WithValue(ctx, oauthKey, oauth)
}

func GetUser(ctx context.Context) (User, error) {
	if ctx == nil {
		return User{}, fmt.Errorf("cannot get user: context is nil")
	}
	if user, ok := ctx.Value(userKey).(*User); ok && user != nil {
		return user.Clone(), nil
	}
	if authErr, ok := ctx.Value(authErrKey).(error); ok {
		return User{}, fmt.Errorf("cannot get user: %w", authErr)
	}
	return User{}, fmt.Errorf("cannot get user: context doesn't contain user data")
}

func GetUserTicket(ctx context.Context) string {
	if ctx == nil {
		return ""
	}
	if user, ok := ctx.Value(userKey).(*User); ok {
		return user.Ticket
	}
	return ""
}

func GetOAuth(ctx context.Context) (OAuth, error) {
	var o OAuth
	if ctx == nil {
		return o, fmt.Errorf("cannot get oauth: context is nil")
	}
	if oauth, ok := ctx.Value(oauthKey).(OAuth); ok {
		return oauth, nil
	}
	return o, fmt.Errorf("cannot get oauth: context doesn't contain oauth data")
}
