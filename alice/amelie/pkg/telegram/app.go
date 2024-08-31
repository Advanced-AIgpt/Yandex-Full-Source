package telegram

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/amelie/pkg/util"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type telegramApp struct {
	logger        log.Logger
	serviceLogger log.Logger
	controller    Controller
}

func (app *telegramApp) OnText(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, TextEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnText(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnCallback(ctx context.Context, bot Bot, cb *Callback) {
	app.onEvent(ctx, bot, CallbackEvent, cb, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnCallback(ctx, bot, event.(*Callback))
	})
}

func (app *telegramApp) OnVoice(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, VoiceEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnVoice(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnLocation(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, LocationEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnLocation(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnPhoto(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, PhotoEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnPhoto(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnDocument(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, DocumentEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnDocument(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnQuery(ctx context.Context, bot Bot, query *Query) {
	app.onEvent(ctx, bot, QueryEvent, query, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnQuery(ctx, bot, event.(*Query))
	})
}

func (app *telegramApp) OnPinned(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, PinnedEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnPinned(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnAudio(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, AudioEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnAudio(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnAnimation(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, AnimationEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnAnimation(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnSticker(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, StickerEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnSticker(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnVideo(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, VideoEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnVideo(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnVideoNote(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, VideoNoteEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnVideoNote(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnContact(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, ContactEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnContact(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnVenue(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, VenueEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnVenue(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnDice(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, DiceEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnDice(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnInvoice(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, InvoiceEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnInvoice(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnPayment(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, PaymentEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnPayment(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnAddedToGroup(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, AddedToGroupEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnAddedToGroup(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnUserJoined(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, UserJoinedEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnUserJoined(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnUserLeft(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, UserLeftEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnUserLeft(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnNewGroupTitle(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, NewGroupTitleEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnNewGroupTitle(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnNewGroupPhoto(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, NewGroupPhotoEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnNewGroupPhoto(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnGroupPhotoDeleted(ctx context.Context, bot Bot, msg *Message) {
	app.onEvent(ctx, bot, GroupPhotoDeletedEvent, msg, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnGroupPhotoDeleted(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnEdited(ctx context.Context, bot Bot, message *Message) {
	app.onEvent(ctx, bot, EditedEvent, message, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnEdited(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnChannelPost(ctx context.Context, bot Bot, post *Message) {
	app.onEvent(ctx, bot, ChannelPostEvent, post, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnChannelPost(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnEditedChannelPost(ctx context.Context, bot Bot, post *Message) {
	app.onEvent(ctx, bot, EditedChannelPostEvent, post, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnEditedChannelPost(ctx, bot, event.(*Message))
	})
}

func (app *telegramApp) OnChosenInlineResult(ctx context.Context, bot Bot, result *ChosenInlineResult) {
	app.onEvent(ctx, bot, ChosenInlineResultEvent, result, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnChosenInlineResult(ctx, bot, event.(*ChosenInlineResult))
	})
}

func (app *telegramApp) OnShipping(ctx context.Context, bot Bot, query *ShippingQuery) {
	app.onEvent(ctx, bot, ShippingEvent, query, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnShipping(ctx, bot, event.(*ShippingQuery))
	})
}

func (app *telegramApp) OnCheckout(ctx context.Context, bot Bot, query *PreCheckoutQuery) {
	app.onEvent(ctx, bot, CheckoutEvent, query, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnCheckout(ctx, bot, event.(*PreCheckoutQuery))
	})
}

func (app *telegramApp) OnPoll(ctx context.Context, bot Bot, poll *Poll) {
	app.onEvent(ctx, bot, PollEvent, poll, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnPoll(ctx, bot, event.(*Poll))
	})
}

func (app *telegramApp) OnPollAnswer(ctx context.Context, bot Bot, answer *PollAnswer) {
	app.onEvent(ctx, bot, PollAnswerEvent, answer, func(ctx context.Context, bot Bot, event interface{}) {
		app.controller.OnPollAnswer(ctx, bot, event.(*PollAnswer))
	})
}

func (app *telegramApp) onEvent(ctx context.Context, bot Bot, eventType EventType, event interface{},
	eventHandler func(ctx context.Context, bot Bot, event interface{})) {
	defer func() {
		if rec := recover(); rec != nil {
			msg := fmt.Sprintf("fatal panic recovered: %v", rec)
			ctxlog.Error(ctx, app.serviceLogger, msg)
			setrace.ErrorLogEvent(ctx, app.logger, msg)
		}
	}()
	meta, err := getEventMeta(eventType, event)
	if err != nil {
		msg := fmt.Sprintf("event meta error: %v", err)
		ctxlog.Error(ctx, app.serviceLogger, msg)
		setrace.ErrorLogEvent(ctx, app.logger, msg)
		// TODO: fallback
		return
	}
	setrace.InfoLogEvent(ctx, app.logger, "Received event", log.String("event_type", string(eventType)),
		log.Any("event", event))
	ctx = withEventMeta(ctx, meta)
	defer func() {
		if rec := recover(); rec != nil {
			msg := fmt.Sprintf("event handler panic recovered: %v", rec)
			ctxlog.Error(ctx, app.serviceLogger, msg)
			setrace.ErrorLogEvent(ctx, app.logger, msg)
		}
	}()
	if err := app.controller.GetInterceptor().
		Intercept(ctx, bot, eventType, event, func(ctx context.Context, bot Bot, eventType EventType, event interface{}) error {
			setrace.InfoLogEvent(ctx, app.logger, "Processing event handler")
			eventHandler(ctx, bot, event)
			return nil
		}); err != nil {
		msg := fmt.Sprintf("interceptor error: %v", err)
		ctxlog.Error(ctx, app.serviceLogger, msg)
		setrace.ErrorLogEvent(ctx, app.logger, msg)
	}
}

func NewApp(logger log.Logger, serviceLogger log.Logger, controller Controller) (App, error) {
	if err := util.ValidateNotNil(logger, serviceLogger, controller); err != nil {
		return nil, fmt.Errorf("new app arguments error: %w", err)
	}
	return &telegramApp{
		logger:        logger,
		serviceLogger: serviceLogger,
		controller:    controller,
	}, nil
}
