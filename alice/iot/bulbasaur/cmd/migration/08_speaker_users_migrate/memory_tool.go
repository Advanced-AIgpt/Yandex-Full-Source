package main

import (
	"context"
	"sync"

	"github.com/gofrs/uuid"
)

type IotUserMemoryTool struct {
	users *sync.Map
}

func NewUserMemoryTool(ctx context.Context, client *DBClient) IotUserMemoryTool {
	tool := IotUserMemoryTool{
		users: new(sync.Map),
	}
	var uuidCount int
	for user := range client.StreamUsers(ctx) {
		uuidCount++
		tool.users.Store(user.ID, user)
	}
	logger.Infof("Memoized %d users", uuidCount)
	return tool
}

func (tool IotUserMemoryTool) GetUser(id uint64) IotUser {
	if value, isKnown := tool.users.Load(id); isKnown {
		return value.(IotUser)
	}
	iu := IotUser{ID: id}
	tool.users.Store(id, iu)
	return iu
}

type IotDeviceMemoryTool struct {
	devices *sync.Map
}

type UniquePair struct {
	HUID  uint64
	ExtID string
}

func NewDeviceMemoryTool(ctx context.Context, client *DBClient) IotDeviceMemoryTool {
	tool := IotDeviceMemoryTool{
		devices: new(sync.Map),
	}
	var uuidCount int
	for speaker := range client.StreamSpeakerDevices(ctx) {
		uuidCount++
		tool.devices.Store(UniquePair{speaker.HUID, speaker.externalID}, speaker)
	}
	logger.Infof("Memoized %d speakers", uuidCount)
	return tool
}

func (tool IotDeviceMemoryTool) GetDevice(uniqueID UniquePair) IotDevice {
	if value, isKnown := tool.devices.Load(uniqueID); isKnown {
		return value.(IotDevice)
	}
	UUID, _ := uuid.NewV4()
	stringUUID := UUID.String()
	sd := IotDevice{ID: stringUUID}
	tool.devices.Store(uniqueID, sd)
	return sd
}
