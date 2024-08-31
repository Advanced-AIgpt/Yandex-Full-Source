package userapi

import (
	"context"
)

type APIClient interface {
	GetUserProfile(ctx context.Context, token string) (UserProfileResult, error)
}

type APIClientMock struct {
	GetUserProfileFunc func(ctx context.Context, token string) (UserProfileResult, error)
}

func (a APIClientMock) GetUserProfile(ctx context.Context, token string) (UserProfileResult, error) {
	return a.GetUserProfileFunc(ctx, token)
}
