package iotapi

import (
	"context"
)

type ctxKeySignal int

const (
	signalKey ctxKeySignal = iota
)

const (
	getUserDevicesSignal = iota
	getUserHomesSignal
	getUserDeviceInfoSignal
	getPropertiesSignal
	setPropertiesSignal
	setActionsSignal
	subscribeToPropertiesChangedSignal
	subscribeToUserEventsSignal
	subscribeToDeviceEventsSignal
)

func withGetUserDevicesSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, getUserDevicesSignal)
	return ctx
}

func withGetUserHomesSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, getUserHomesSignal)
	return ctx
}

func withGetUserDeviceInfoSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, getUserDeviceInfoSignal)
	return ctx
}

func withGetPropertiesSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, getPropertiesSignal)
	return ctx
}

func withSetPropertiesSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, setPropertiesSignal)
	return ctx
}

func withSetActionsSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, setActionsSignal)
	return ctx
}

func withSubscribeToPropertiesChangedSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, subscribeToPropertiesChangedSignal)
	return ctx
}

func withSubscribeToUserEventsSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, subscribeToUserEventsSignal)
	return ctx
}

func withSubscribeToDeviceEventsSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, subscribeToDeviceEventsSignal)
	return ctx
}
