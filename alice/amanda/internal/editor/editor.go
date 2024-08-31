package editor

import (
	"encoding/json"
	"fmt"
	"html/template"
	"strings"

	"go.uber.org/zap"

	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/library/go/core/resource"
)

const (
	_deviceStateEditorTemplatePath = "templates/device_state_editor.html"

	_deviceEditor = "DeviceEditor"
)

type Editor struct {
	deviceTemplate *template.Template
	sugar          *zap.SugaredLogger
}

func New(sugar *zap.SugaredLogger) *Editor {
	return &Editor{
		deviceTemplate: template.Must(
			template.New(_deviceEditor).
				Parse(string(resource.Get(_deviceStateEditorTemplatePath))),
		),
		sugar: sugar,
	}
}

type DeviceStateTemplate struct {
	DeviceState string
	Message     string
}

func (editor *Editor) getDeviceState(tmpl DeviceStateTemplate) (string, error) {
	builder := strings.Builder{}
	if err := editor.deviceTemplate.ExecuteTemplate(&builder, _deviceEditor, tmpl); err != nil {
		return "", err
	}
	return builder.String(), nil
}

func (editor *Editor) getLogger(sess *session.Session) *zap.SugaredLogger {
	return editor.sugar.With("uuid", sess.Settings.ApplicationDetails.UUID)
}

func (editor *Editor) GetEditDeviceState(sess *session.Session) (string, error) {
	editor.getLogger(sess).Info("on edit device state")
	return editor.getDeviceState(DeviceStateTemplate{
		DeviceState: sess.Settings.DeviceState,
	})
}

func (editor *Editor) UpdateDeviceState(sess *session.Session, deviceState string) (string, error) {
	editor.getLogger(sess).Infof("updating device state to %s", deviceState)
	onInvalidDeviceState := func(err error) (string, error) {
		return editor.getDeviceState(DeviceStateTemplate{
			DeviceState: sess.Settings.DeviceState,
			Message:     fmt.Sprintf("Невалидный Device State: %v", err),
		})
	}
	if deviceState == "" {
		deviceState = "{}"
	}
	buffer := map[string]interface{}{}
	if err := json.Unmarshal([]byte(deviceState), &buffer); err != nil {
		return onInvalidDeviceState(err)
	}
	data, err := json.Marshal(buffer)
	if err != nil {
		return onInvalidDeviceState(err)
	}
	sess.Settings.DeviceState = string(data)
	return editor.getDeviceState(DeviceStateTemplate{
		DeviceState: sess.Settings.DeviceState,
		Message:     "Device State успешно обновлен",
	})
}
