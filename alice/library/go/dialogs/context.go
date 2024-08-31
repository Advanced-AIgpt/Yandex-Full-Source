package dialogs

import "context"

type ctxKeyDialogAuth int

const dialogAuthKey ctxKeyDialogAuth = 0

func WithDialogAuthData(ctx context.Context, data AuthorizationData) context.Context {
	return context.WithValue(ctx, dialogAuthKey, data)
}

func GetDialogAuthData(ctx context.Context) AuthorizationData {
	if ctx == nil {
		return AuthorizationData{}
	}
	if authData, ok := ctx.Value(dialogAuthKey).(AuthorizationData); ok {
		return authData
	}
	return AuthorizationData{}
}
