package telegram

import (
	"context"

	tb "gopkg.in/tucnak/telebot.v2"
)

type (
	App interface {
		EventProcessor
	}
	Bot interface {
		GetMe() *tb.User
		Send(ctx context.Context, to Recipient, what interface{}, options ...interface{}) (*Message, error)
		Reply(ctx context.Context, what interface{}, options ...interface{}) (*Message, error)
		Edit(ctx context.Context, msg *Message, what interface{}, options ...interface{}) (*Message, error)
		Delete(ctx context.Context, msg *Message) error
		SetCommands(ctx context.Context, commands []Command) error
		Respond(ctx context.Context, cb *Callback, response ...*CallbackResponse) error
	}
	Controller interface {
		InterceptorManager
		EventProcessor
	}
	InterceptorManager interface {
		GetInterceptor() Interceptor
	}
	Interceptor interface {
		Intercept(ctx context.Context, bot Bot, eventType EventType, event interface{}, next NextInterceptorDelegate) error
	}
	NextInterceptorDelegate func(ctx context.Context, bot Bot, eventType EventType, event interface{}) error
	Recipient               interface {
		tb.Recipient
	}
	EventProcessor interface {
		OnText(ctx context.Context, bot Bot, msg *Message)
		OnCallback(ctx context.Context, bot Bot, cb *Callback)
		OnVoice(ctx context.Context, bot Bot, msg *Message)
		OnLocation(ctx context.Context, bot Bot, msg *Message)
		OnPhoto(ctx context.Context, bot Bot, msg *Message)
		OnDocument(ctx context.Context, bot Bot, msg *Message)
		OnQuery(ctx context.Context, bot Bot, query *Query)
		OnPinned(ctx context.Context, bot Bot, msg *Message)
		OnAudio(ctx context.Context, bot Bot, msg *Message)
		OnAnimation(ctx context.Context, bot Bot, msg *Message)
		OnSticker(ctx context.Context, bot Bot, msg *Message)
		OnVideo(ctx context.Context, bot Bot, msg *Message)
		OnVideoNote(ctx context.Context, bot Bot, msg *Message)
		OnContact(ctx context.Context, bot Bot, msg *Message)
		OnVenue(ctx context.Context, bot Bot, msg *Message)
		OnDice(ctx context.Context, bot Bot, msg *Message)
		OnInvoice(ctx context.Context, bot Bot, msg *Message)
		OnPayment(ctx context.Context, bot Bot, msg *Message)
		OnAddedToGroup(ctx context.Context, bot Bot, msg *Message)
		OnUserJoined(ctx context.Context, bot Bot, msg *Message)
		OnUserLeft(ctx context.Context, bot Bot, msg *Message)
		OnNewGroupTitle(ctx context.Context, bot Bot, msg *Message)
		OnNewGroupPhoto(ctx context.Context, bot Bot, msg *Message)
		OnGroupPhotoDeleted(ctx context.Context, bot Bot, msg *Message)
		OnEdited(ctx context.Context, bot Bot, message *Message)
		OnChannelPost(ctx context.Context, bot Bot, post *Message)
		OnEditedChannelPost(ctx context.Context, bot Bot, post *Message)
		OnChosenInlineResult(ctx context.Context, bot Bot, result *ChosenInlineResult)
		OnShipping(ctx context.Context, bot Bot, query *ShippingQuery)
		OnCheckout(ctx context.Context, bot Bot, query *PreCheckoutQuery)
		OnPoll(ctx context.Context, bot Bot, poll *Poll)
		OnPollAnswer(ctx context.Context, bot Bot, answer *PollAnswer)
	}
)
