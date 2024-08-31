package telegram

import (
	"fmt"
)

func getEventMeta(eventType EventType, event interface{}) (eventMeta EventMeta, err error) {
	switch eventType {
	case QueryEvent:
		eventMeta, err = getEventMetaFromQuery(event)
	case CallbackEvent:
		eventMeta, err = getEventMetaFromCallback(event)
	default:
		eventMeta, err = getEventMetaFromMessage(event)
	}
	if err == nil {
		eventMeta.EventType = eventType
	}
	return
}

func getEventMetaFromMessage(event interface{}) (EventMeta, error) {
	msg, ok := event.(*Message)
	if !ok {
		return EventMeta{}, fmt.Errorf("invalid event type, expected *Message but found: %+v", event)
	}
	return EventMeta{
		Username: msg.Sender.Username,
		ChatID:   msg.Chat.ID,
		UserID:   int64(msg.Sender.ID),
	}, nil
}

func getEventMetaFromCallback(event interface{}) (EventMeta, error) {
	cb, ok := event.(*Callback)
	if !ok {
		return EventMeta{}, fmt.Errorf("invalid event type, expected *Callback but found: %+v", event)
	}
	return EventMeta{
		Username: cb.Sender.Username,
		ChatID:   cb.Message.Chat.ID,
		UserID:   int64(cb.Sender.ID),
	}, nil
}

func getEventMetaFromQuery(event interface{}) (EventMeta, error) {
	query, ok := event.(*Query)
	if !ok {
		return EventMeta{}, fmt.Errorf("invalid event type, expected *Query but found: %+v", event)
	}
	return EventMeta{
		Username: query.From.Username,
		ChatID:   0,
		UserID:   int64(query.From.ID),
	}, nil
}
