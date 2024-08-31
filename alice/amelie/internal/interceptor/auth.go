package interceptor

import (
	"context"
	"fmt"
	"strings"
	"time"

	tb "gopkg.in/tucnak/telebot.v2" // todo: remove dep

	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/passport"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	greetings = `Привет!
Я могу помочь Вам в управлении колонками с Алисой, но для начала нужно войти в аккаунт:`
	greetingsContinuation = `Для того, чтобы войти в аккаунт нажмите на кнопку:`
	authError             = `Произошла ошибка во время авторизации, свяжитесь с технической поддержкой или попробуйте еще раз.`
	success               = `Вы успешно авторизовались как %s.`
)

const (
	day = time.Hour * 24
)

type AuthInterceptor struct {
	oauthClient        passport.OAuthClient
	passportClient     passport.Client
	sessionInterceptor *SessionInterceptor
	logger             log.Logger
}

func (i *AuthInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	session := i.sessionInterceptor.GetSession(ctx)
	actualAccounts := []model.Account{}
	for _, account := range session.User.Accounts {
		if time.Now().After(account.TokenDeadlineTime) {
			setrace.InfoLogEvent(ctx, i.logger, fmt.Sprintf("Account %s dropped: token is out of date", account.Username))
			continue
		}
		if time.Now().Add(time.Hour * 24 * 7).After(account.TokenDeadlineTime) {
			r, err := i.oauthClient.RefreshOAuthToken(ctx, account.RefreshToken)
			if err != nil {
				setrace.ErrorLogEvent(ctx, i.logger, fmt.Sprintf("Failed to refresh user token: %v", err))
				continue
			}
			actualAccounts = append(actualAccounts, i.newAccount(ctx, r))
			continue
		}
		actualAccounts = append(actualAccounts, account)
	}
	session.User.Accounts = actualAccounts

	if len(session.User.Accounts) == 0 {
		// TODO: update token
		if eventType == telegram.TextEvent {
			msg := event.(*telegram.Message)
			if strings.HasPrefix(msg.Text, "/start ") {
				code := strings.TrimPrefix(msg.Text, "/start ")
				if err := i.AddAccount(ctx, code); err != nil {
					_, _ = bot.Reply(ctx, authError)
					return err
				}
				_, _ = bot.Reply(ctx, fmt.Sprintf(success, session.User.Accounts[0].Username))
				msg.Text = "/help"
				return next(ctx, bot, eventType, msg)
			}
		}
		i.showGreetings(ctx, session, bot)
		return nil
	}
	session.User.Username = telegram.GetEventMeta(ctx).Username
	return next(ctx, bot, eventType, event)
}

func NewAuthInterceptor(oauthClient passport.OAuthClient, passportClient passport.Client, sessionInterceptor *SessionInterceptor, logger log.Logger) *AuthInterceptor {
	// todo: validate
	return &AuthInterceptor{
		oauthClient:        oauthClient,
		passportClient:     passportClient,
		sessionInterceptor: sessionInterceptor,
		logger:             logger,
	}
}

func (i *AuthInterceptor) GetOAuthURL(bot telegram.Bot) string {
	return i.oauthClient.GenerateOAuthCallbackURL(bot.GetMe().Username)
}

func (i *AuthInterceptor) makeOAuthReplyMarkup(bot telegram.Bot) *tb.ReplyMarkup {
	return &tb.ReplyMarkup{
		InlineKeyboard: [][]tb.InlineButton{{
			{
				Text: "Войти через Yandex",
				URL:  i.GetOAuthURL(bot),
			},
		},
		},
	}
}

func (i *AuthInterceptor) showGreetings(ctx context.Context, session *model.Session, bot telegram.Bot) {
	now := time.Now()
	if now.Sub(session.User.LastGreetingsShowTime) > day {
		_, _ = bot.Reply(ctx, greetings, i.makeOAuthReplyMarkup(bot))
		session.User.LastGreetingsShowTime = now
	} else {
		i.ShowLoginInfo(ctx, bot)
	}
}

func (i *AuthInterceptor) ShowLoginInfo(ctx context.Context, bot telegram.Bot) {
	_, _ = bot.Reply(ctx, greetingsContinuation, i.makeOAuthReplyMarkup(bot))
}

func (i *AuthInterceptor) newAccount(ctx context.Context, r passport.OAuthResponse) model.Account {
	acc := model.Account{
		Username:          "",
		Token:             r.AccessToken,
		RefreshToken:      r.RefreshToken,
		TokenDeadlineTime: time.Now().Add(time.Duration(r.ExpiresIn) * time.Second),
	}
	info, err := i.passportClient.GetUserInfo(ctx, r.AccessToken)
	if err != nil {
		setrace.InfoLogEvent(ctx, i.logger, fmt.Sprintf("get user info error: %v", err))
	} else {
		acc.Username = info.Username
		acc.ID = info.UserID
	}
	return acc
}

func (i *AuthInterceptor) AddAccount(ctx context.Context, code string) error {
	r, err := i.oauthClient.GetOAuthToken(ctx, code)
	if err != nil {
		return err
	}
	session := i.sessionInterceptor.GetSession(ctx)
	session.User.Accounts = append(session.User.Accounts, i.newAccount(ctx, r))
	return nil
}
