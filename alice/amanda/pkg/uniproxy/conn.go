package uniproxy

import (
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"strings"
	"time"

	"github.com/gorilla/websocket"
	"github.com/mitchellh/mapstructure"

	"a.yandex-team.ru/alice/amanda/internal/uuid"
	"a.yandex-team.ru/alice/amanda/pkg/speechkit"
	"a.yandex-team.ru/alice/amanda/pkg/uniproxy/internal"
)

// todo: add timeouts

const (
	_streamID = 1
)

const (
	_system internal.Namespace = "System"
	_vins   internal.Namespace = "Vins"
)

type Event string

const (
	_synchronizeState Event = "SynchronizeState"
	_textInput        Event = "TextInput"
	_voiceInput       Event = "VoiceInput"
	_customInput      Event = "CustomInput"
	_vinsResponse     Event = "VinsResponse"
	_eventException   Event = "EventException"
)

var (
	ErrEventException = errors.New("event exception")
	ErrEmptyASRResult = errors.New("empty asr result")
)

func getStringWithDefault(v, d string) string {
	if v == "" {
		return d
	}
	return v
}

func getUtteranceFromASRResult(response *internal.ASRResult) string {
	recognition := response.Recognition[len(response.Recognition)-1]
	words := make([]string, 0, len(recognition.Words))
	for _, word := range recognition.Words {
		words = append(words, word.Value)
	}
	return strings.Join(words, " ")
}

type Conn struct {
	ws       *websocket.Conn
	settings Settings
}

func (conn *Conn) sendSynchronizeState() error {
	return conn.sendEvent(_system, _synchronizeState, &internal.SynchronizeState{
		AuthToken:   conn.settings.AuthToken,
		UUID:        conn.settings.UUID,
		Lang:        conn.settings.Language,
		Voice:       conn.settings.Voice,
		VINSPartial: false,
		VINSURL:     conn.settings.VINSURL,
		OAuthToken: func() *string {
			token := conn.settings.AdditionalOptions.OAuthToken
			if len(token) > 0 {
				return &token
			}
			return nil
		}(),
		SpeechKitVersion: nil,
		Device:           nil,
		PlatformInfo:     nil,
		NetworkType:      nil,
		SaveToMDS:        nil,
		DisableFallback:  nil,
	})
}

func (conn *Conn) sendText(text string) (response *Response, err error) {
	if err := conn.sendTextEvent(text); err != nil {
		return nil, err
	}
	return conn.receiveResponse()
}

func (conn *Conn) sendTextEvent(text string) error {
	return conn.sendEvent(_vins, _textInput, internal.SpeechKitRequest{
		Request: speechkit.Request{
			Application: conn.makeApp(),
			Header:      conn.makeHeader(),
			Body: conn.makeBody(speechkit.Event{
				Type: speechkit.TextInput,
				Text: text,
			}),
		},
	})
}

func (conn *Conn) sendEvent(ns internal.Namespace, event Event, payload interface{}) error {
	_ = conn.ws.SetWriteDeadline(time.Now().Add(time.Second * 15)) // todo: customize
	var streamID uint32 = _streamID
	messageID := uuid.New()
	fmt.Printf("Message ID: %s\n", messageID)
	return conn.sendJSON(internal.Message{
		Event: &internal.Event{
			Header: internal.EventHeader{
				Namespace: ns,
				Name:      string(event),
				MessageID: messageID,
				StreamID: func() *uint32 {
					if event == _synchronizeState {
						return nil
					}
					return &streamID
				}(),
			},
			Payload: payload,
		},
	})
}

func (conn *Conn) sendBinary(data []byte) error {
	msg := make([]byte, 4)
	binary.BigEndian.PutUint32(msg, _streamID)
	msg = append(msg, data...)
	return conn.ws.WriteMessage(websocket.BinaryMessage, msg)
}

func (conn *Conn) readJSON(v interface{}) error {
	if err := conn.ws.ReadJSON(v); err != nil {
		return err
	}
	return nil
}

func (conn *Conn) sendJSON(v interface{}) error {
	return conn.ws.WriteJSON(v)
}

func (conn *Conn) close() error {
	return conn.ws.Close()
}

func (conn *Conn) makeApp() *speechkit.App {
	return &conn.settings.App
}

func (conn *Conn) makeHeader() *speechkit.Header {
	return &conn.settings.Header
}

func (conn *Conn) makeBody(event speechkit.Event) *speechkit.RequestBody {
	return &speechkit.RequestBody{
		Event:             &event,
		Location:          &conn.settings.Location,
		Experiments:       conn.settings.Experiments,
		DeviceState:       &conn.settings.DeviceState,
		AdditionalOptions: &conn.settings.AdditionalOptions,
		VoiceSession:      conn.settings.VoiceSession,
		ResetSession:      &conn.settings.ResetSession,
		LAASRegion:        &conn.settings.LAASRegion,
	}
}

func (conn *Conn) receiveResponse() (response *Response, err error) {
	response = new(Response)
	response.SKResponse, err = conn.receiveSKResponse()
	if err != nil {
		return nil, err
	}
	if conn.settings.SkipTTS || !isVoiceResponseExpected(response.SKResponse) {
		return response, nil
	}
	response.TTSResponse, err = conn.receiveTTSResponse()
	if err != nil {
		return response, err
	}
	return response, nil
}

func (conn *Conn) receiveSKResponse() (*speechkit.Response, error) {
	const readEventTimout = time.Second * 5
	for {
		msg := &internal.Message{Directive: &internal.Event{Payload: map[string]interface{}{}}}
		_ = conn.ws.SetReadDeadline(time.Now().Add(readEventTimout)) // TODO: customize
		if err := conn.readJSON(&msg); err != nil {
			return nil, fmt.Errorf("failed to get response from uniproxy: %w", checkTimeoutError(err, readEventTimout))
		}
		if msg.Directive == nil {
			continue
		}
		if msg.Directive.Header.Namespace == _system && msg.Directive.Header.Name == string(_eventException) {
			return nil, fmt.Errorf("%s: %w", getEventExceptionMessage(msg), ErrEventException)
		}
		if msg.Directive.Header.Namespace == _vins && msg.Directive.Header.Name == string(_vinsResponse) {
			response := new(speechkit.Response)
			if err := json2struct(msg.Directive.Payload.(map[string]interface{}), response); err != nil {
				return nil, fmt.Errorf("unable to decode response: %w", err)
			}
			return response, nil
		}
	}
}

type TTSResponseOrError struct {
	internal.Message
	speechkit.TTSResponse
}

func lookupEventExceptionMessage(msg map[string]interface{}) (s string) {
	defer func() {
		if r := recover(); r != nil {
			s = fmt.Sprintf("%#v", msg)
		}
	}()
	s = msg["error"].(map[string]interface{})["message"].(string)
	return
}

func getEventExceptionMessage(msg *internal.Message) string {
	if m, ok := msg.Directive.Payload.(map[string]interface{}); ok {
		return lookupEventExceptionMessage(m)
	}
	return fmt.Sprintf("%#v", msg)
}

func (conn *Conn) receiveTTSResponse() (*speechkit.TTSResponse, error) {
	response := TTSResponseOrError{}
	if err := conn.readJSON(&response); err != nil {
		return nil, err
	}
	if response.Directive != nil && response.Directive.Header.Name == string(_eventException) {
		return nil, fmt.Errorf("%s: %w", getEventExceptionMessage(&response.Message), ErrEventException)
	}
	const receiveTTSTimeout = time.Second * 20
	_ = conn.ws.SetReadDeadline(time.Now().Add(receiveTTSTimeout)) // TODO: customize
	for {
		messageType, data, err := conn.ws.ReadMessage()
		if err != nil {
			return nil, fmt.Errorf("receive tts response: %w", checkTimeoutError(err, receiveTTSTimeout))
		}
		if messageType != websocket.BinaryMessage {
			break
		}
		if len(data) < 4 {
			continue
		}
		response.Data = append(response.Data, data[4:]...)
	}
	// TODO: add end stream validation
	return &response.TTSResponse, nil
}

func (conn *Conn) sendVoice(data []byte) (*Response, error) {
	if err := conn.sendVoiceEvent(data); err != nil {
		return nil, err
	}
	asrResult, err := conn.receiveASRResult()
	if err != nil {
		return nil, err
	}
	response, err := conn.receiveResponse()
	utterance := getUtteranceFromASRResult(asrResult)
	if err != nil {
		return &Response{ASRText: &utterance}, err
	}
	response.ASRText = &utterance
	return response, err
}

func (conn *Conn) sendCallback(name string, payload interface{}) (*Response, error) {
	if err := conn.sendCallbackEvent(name, payload); err != nil {
		return nil, err
	}
	return conn.receiveResponse()
}

func (conn *Conn) sendVoiceEvent(data []byte) error {
	err := conn.sendEvent(_vins, _voiceInput, internal.SpeechKitRequest{
		Request: speechkit.Request{
			Application: conn.makeApp(),
			Header:      conn.makeHeader(),
			Body: conn.makeBody(speechkit.Event{
				Type: speechkit.VoiceInput,
			}),
		},
		Topic:  getStringWithDefault(conn.settings.ASRTopic, "general"),
		Format: "audio/opus",
		AdvancedASROptions: &internal.AdvancedASROptions{
			UtteranceSilence: 40,
		},
	})
	if err != nil {
		return err
	}
	if err := conn.sendBinary(data); err != nil {
		return err
	}
	return conn.sendStreamControl(internal.CloseStream, internal.NoError)
}

func (conn *Conn) receiveASRResult() (asrResult *internal.ASRResult, err error) {
	asrResult = new(internal.ASRResult)
	const receiveASRTimeout = time.Second * 10
	for {
		_ = conn.ws.SetReadDeadline(time.Now().Add(receiveASRTimeout)) // TODO: customize
		msg := map[string]interface{}{}
		if err := conn.readJSON(&msg); err != nil {
			return nil, fmt.Errorf("receive asr result: %w", checkTimeoutError(err, receiveASRTimeout))
		}
		if directive, ok := msg["directive"]; ok {
			if err := mapstructure.Decode(directive.(map[string]interface{})["payload"], asrResult); err != nil {
				return nil, fmt.Errorf("unable to parse asr result: %v", err)
			}
			continue
		}
		if _, ok := msg["streamcontrol"]; ok {
			break
		}
		return nil, fmt.Errorf("expected stream control or directive, but got %#v", msg)
	}
	if len(asrResult.Recognition) == 0 || len(asrResult.Recognition[len(asrResult.Recognition)-1].Words) == 0 {
		return nil, ErrEmptyASRResult
	}
	return asrResult, nil
}

func (conn *Conn) sendStreamControl(action internal.Action, reason internal.Reason) error {
	return conn.sendJSON(internal.Message{
		StreamControl: &internal.StreamControl{
			MessageID: uuid.New(),
			Action:    action,
			Reason:    reason,
			StreamID:  _streamID,
		},
	})
}

func (conn *Conn) sendCallbackEvent(name string, payload interface{}) error {
	return conn.sendEvent(_vins, _customInput, internal.SpeechKitRequest{
		Request: speechkit.Request{
			Application: conn.makeApp(),
			Header:      conn.makeHeader(),
			Body: conn.makeBody(speechkit.Event{
				Type:    speechkit.ServerAction,
				Name:    name,
				Payload: payload,
			}),
		},
	})
}

func (conn *Conn) sendImage(url string) (*Response, error) {
	if err := conn.sendImageEvent(url); err != nil {
		return nil, err
	}
	return conn.receiveResponse()
}

func (conn *Conn) sendImageEvent(url string) error {
	return conn.sendEvent(_vins, _customInput, internal.SpeechKitRequest{
		Request: speechkit.Request{
			Application: conn.makeApp(),
			Header:      conn.makeHeader(),
			Body: conn.makeBody(speechkit.Event{
				Type: speechkit.ImageInput,
				Payload: map[string]string{
					"img_url": url,
				},
			}),
		},
	})
}

func newConn(settings Settings) (conn *Conn, err error) {
	conn = new(Conn)
	conn.settings = settings
	conn.ws, _, err = websocket.DefaultDialer.Dial(settings.UniproxyURL, nil)
	if err != nil {
		return nil, err
	}
	return
}

func isVoiceResponseExpected(r *speechkit.Response) bool {
	if r != nil && r.VoiceResponse != nil && r.VoiceResponse.OutputSpeech != nil {
		return r.VoiceResponse.OutputSpeech.Text != ""
	}
	return false
}

func json2struct(raw map[string]interface{}, obj interface{}) error {
	data, err := json.Marshal(raw)
	if err != nil {
		return err
	}
	return json.Unmarshal(data, obj)
}

func checkTimeoutError(err error, timeout time.Duration) error {
	if strings.Contains(err.Error(), "i/o timeout") {
		return fmt.Errorf("timeout of %s exceeded", timeout)
	}
	return err
}
