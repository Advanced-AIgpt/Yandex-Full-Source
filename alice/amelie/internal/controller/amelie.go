package controller

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/bass"
	"a.yandex-team.ru/alice/amelie/pkg/iot"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	maxTextLen = 1024
)

type Amelie struct {
	logger log.Logger

	sessionInterceptor *interceptor.SessionInterceptor
	authInterceptor    *interceptor.AuthInterceptor
	yandexInterceptor  *interceptor.YandexInterceptor
	stateInterceptor   *interceptor.StateInterceptor
	commandInterceptor *interceptor.CommandInterceptor
	cancelInterceptor  *interceptor.CancelInterceptor
	sensorInterceptor  *interceptor.SensorInterceptor
	bassClient         bass.Client

	deviceController       *deviceController
	youTubeController      *youTubeController
	accountController      *accountController
	helpController         *helpCommand
	rateLimiterInterceptor *interceptor.RateLimiterInterceptor
}

type IoTClientFactory func(token string) iot.Client

type interceptorComposition struct {
	interceptors []telegram.Interceptor
	logger       log.Logger
}

func (i *interceptorComposition) wrapInterceptor(inter telegram.Interceptor, next telegram.NextInterceptorDelegate) telegram.NextInterceptorDelegate {
	return func(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{}) error {
		setrace.InfoLogEvent(ctx, i.logger, fmt.Sprintf("Processing interceptor %T", inter))
		return inter.Intercept(ctx, bot, eventType, event, next)
	}
}

func (i *interceptorComposition) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	if next == nil {
		next = func(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{}) error {
			return nil
		}
	}
	for id := len(i.interceptors) - 1; id >= 0; id-- {
		next = i.wrapInterceptor(i.interceptors[id], next)
	}
	return next(ctx, bot, eventType, event)
}

func (a *Amelie) GetInterceptor() telegram.Interceptor {
	// strict order
	return &interceptorComposition{
		interceptors: []telegram.Interceptor{
			a.sensorInterceptor,
			a.sessionInterceptor,
			a.rateLimiterInterceptor,
			a.authInterceptor,
			a.yandexInterceptor,
			a.cancelInterceptor,
			a.stateInterceptor,
			a.commandInterceptor,
		},
		logger: a.logger,
	}
}

func (a *Amelie) OnText(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
	text := strings.Trim(msg.Text, " ")
	if len(text) == 0 {
		pos := msg.ID % 3
		_, _ = bot.Reply(ctx, "🤔🧐🤨"[pos:pos+1])
		return
	}
	if len(text) > maxTextLen {
		_, _ = bot.Reply(ctx, "Слишком длинное сообщение")
		return
	}
	_ = a.deviceController.AskAlice(ctx, bot, text)
}

func (a *Amelie) OnCallback(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) {
}

func (a *Amelie) OnVoice(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnLocation(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnPhoto(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnDocument(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnQuery(ctx context.Context, bot telegram.Bot, query *telegram.Query) {
}

func (a *Amelie) OnPinned(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnAudio(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnAnimation(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnSticker(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnVideo(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnVideoNote(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnContact(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnVenue(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnDice(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnInvoice(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnPayment(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnAddedToGroup(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnUserJoined(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnUserLeft(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnNewGroupTitle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnNewGroupPhoto(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnGroupPhotoDeleted(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
}

func (a *Amelie) OnEdited(ctx context.Context, bot telegram.Bot, message *telegram.Message) {
}

func (a *Amelie) OnChannelPost(ctx context.Context, bot telegram.Bot, post *telegram.Message) {
}

func (a *Amelie) OnEditedChannelPost(ctx context.Context, bot telegram.Bot, post *telegram.Message) {
}

func (a *Amelie) OnChosenInlineResult(ctx context.Context, bot telegram.Bot, result *telegram.ChosenInlineResult) {
}

func (a *Amelie) OnShipping(ctx context.Context, bot telegram.Bot, query *telegram.ShippingQuery) {
}

func (a *Amelie) OnCheckout(ctx context.Context, bot telegram.Bot, query *telegram.PreCheckoutQuery) {
}

func (a *Amelie) OnPoll(ctx context.Context, bot telegram.Bot, poll *telegram.Poll) {
}

func (a *Amelie) OnPollAnswer(ctx context.Context, bot telegram.Bot, answer *telegram.PollAnswer) {
}

func (a *Amelie) session(ctx context.Context) *model.Session {
	return a.sessionInterceptor.GetSession(ctx)
}

type commandRegistrar interface {
	registerCommands(commandInterceptor *interceptor.CommandInterceptor)
	registerCommandHelpers(add func(info commandHelper))
}

type stateCallbackRegistrar interface {
	registerStateCallback(commandInterceptor *interceptor.StateInterceptor)
}

func (a *Amelie) initCommands() {
	for _, controller := range []interface{}{
		a.accountController,
		a.deviceController,
		a.helpController,
		a.youTubeController,
		//&onboardingController{},
		&shortcutsController{
			sessionManager: a.sessionInterceptor,
			stateManager:   a.stateInterceptor,
		},
		&supportController{},
	} {
		if c, ok := controller.(commandRegistrar); ok {
			c.registerCommands(a.commandInterceptor)
			c.registerCommandHelpers(a.helpController.addHelper)
		}
		if c, ok := controller.(stateCallbackRegistrar); ok {
			c.registerStateCallback(a.stateInterceptor)
		}
	}
	a.helpController.addHelper(commandHelper{
		command:          "/cancel",
		shortDescription: "отменить активную команду",
		longDescription:  "отменить активную команду",
	})
}

func (a *Amelie) GetCommands() []telegram.Command {
	return a.helpController.GetBotCommands()
}

func NewAmelie(logger log.Logger,
	sessionInterceptor *interceptor.SessionInterceptor,
	authInterceptor *interceptor.AuthInterceptor,
	yandexInterceptor *interceptor.YandexInterceptor,
	stateInterceptor *interceptor.StateInterceptor,
	commandInterceptor *interceptor.CommandInterceptor,
	cancelInterceptor *interceptor.CancelInterceptor,
	sensorInterceptor *interceptor.SensorInterceptor,
	rateLimiterInterceptor *interceptor.RateLimiterInterceptor,
	iotClientFactory IoTClientFactory,
	bassClient bass.Client,
) *Amelie {
	amelie := &Amelie{
		logger:                 logger,
		sessionInterceptor:     sessionInterceptor,
		authInterceptor:        authInterceptor,
		yandexInterceptor:      yandexInterceptor,
		stateInterceptor:       stateInterceptor,
		commandInterceptor:     commandInterceptor,
		cancelInterceptor:      cancelInterceptor,
		sensorInterceptor:      sensorInterceptor,
		rateLimiterInterceptor: rateLimiterInterceptor,
		bassClient:             bassClient,
		deviceController: &deviceController{
			iotClientFactory:   iotClientFactory,
			sessionInterceptor: sessionInterceptor,
			stateInterceptor:   stateInterceptor,
			bassClient:         bassClient,
		},
		youTubeController: &youTubeController{
			bassClient:         bassClient,
			sessionInterceptor: sessionInterceptor,
		},
		accountController: &accountController{
			sessionManager: sessionInterceptor,
			authManager:    authInterceptor,
		},
		helpController: &helpCommand{},
	}
	amelie.initCommands()
	return amelie
}
