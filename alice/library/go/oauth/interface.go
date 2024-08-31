package oauth

import "context"

// Client is a client for internal oauth api  https://wiki.yandex-team.ru/oauth/api/
type Client interface {
	// IssueAuthorizationCode requests a new xcode
	// https://wiki.yandex-team.ru/oauth/api/#issueauthorizationcode
	// returns struct with issued xcode and its expiration time
	IssueAuthorizationCode(ctx context.Context,
		userData UserData,
		payload IssueAuthorizationCodeParams,
	) (AuthorizationCode, error)
}
