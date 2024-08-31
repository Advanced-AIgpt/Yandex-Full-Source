package setrace

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"strconv"
	"time"

	"github.com/golang/protobuf/proto"
	"go.uber.org/zap"
	"go.uber.org/zap/buffer"
	"go.uber.org/zap/zapcore"

	setraceproto "a.yandex-team.ru/alice/library/go/setrace/protos"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	rtlog "a.yandex-team.ru/alice/rtlog/protos"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

var _ = zapcore.Encoder(&setraceEncoder{})

var ProtoseqMagicSequence = [...]byte{
	0x1F, 0xF7, 0xF7, 0x7E, 0xBE, 0xA6, 0x5E, 0x9E,
	0x37, 0xA6, 0xF6, 0x2E, 0xFE, 0xAE, 0x47, 0xA7,
	0xB7, 0x6E, 0xBF, 0xAF, 0x16, 0x9E, 0x9F, 0x37,
	0xF6, 0x57, 0xF7, 0x66, 0xA7, 0x06, 0xAF, 0xF7,
}

type setraceEncoder struct {
	environment string
	serviceName string
	hostName    string

	cfg  zapcore.EncoderConfig
	pool buffer.Pool

	namespace     string
	fields        map[string]string
	reflectFields map[string]json.RawMessage

	// for encoding generic values by reflection
	reflectBuf *buffer.Buffer
	reflectEnc *json.Encoder
}

func NewSetraceEncoder(environment, serviceName, hostName string, cfg zapcore.EncoderConfig) *setraceEncoder {
	pool := buffer.NewPool()
	return &setraceEncoder{
		environment: environment,
		serviceName: serviceName,
		hostName:    hostName,

		cfg:           cfg,
		pool:          pool,
		fields:        make(map[string]string),
		reflectFields: make(map[string]json.RawMessage),
	}
}

func (se *setraceEncoder) getKey(key string) string {
	if len(se.namespace) > 0 {
		return se.namespace + "." + key
	}
	return key
}

func (se *setraceEncoder) resetReflectBuf() {
	if se.reflectBuf == nil {
		se.reflectBuf = se.pool.Get()
		se.reflectEnc = json.NewEncoder(se.reflectBuf)
	} else {
		se.reflectBuf.Reset()
	}
}

func (se *setraceEncoder) AddString(key, value string) {
	se.fields[se.getKey(key)] = value
}

func (se *setraceEncoder) OpenNamespace(namespace string) {
	se.namespace = se.getKey(namespace)
}

func (se *setraceEncoder) AddArray(key string, m zapcore.ArrayMarshaler) error {
	return se.AddReflected(key, m)
}

func (se *setraceEncoder) AddObject(key string, m zapcore.ObjectMarshaler) error {
	return se.AddReflected(key, m)
}

func (se *setraceEncoder) AddReflected(key string, value interface{}) error {
	if rawJSON, ok := value.(json.RawMessage); ok {
		se.reflectFields[se.getKey(key)] = rawJSON
		se.fields[fmt.Sprintf("%s__format", se.getKey(key))] = "json"
		return nil
	}
	se.resetReflectBuf()
	if err := se.reflectEnc.Encode(value); err != nil {
		return err
	}
	reflectBytes := se.reflectBuf.Bytes()
	resultBytes := make([]byte, len(reflectBytes))
	copy(resultBytes, reflectBytes) // copy makes buffer reusable
	se.reflectFields[se.getKey(key)] = resultBytes
	se.fields[fmt.Sprintf("%s__format", se.getKey(key))] = "json"
	return nil
}

func (se *setraceEncoder) AddDuration(key string, value time.Duration) {
	se.AddString(key, value.String())
}

func (se *setraceEncoder) AddComplex128(key string, value complex128) {
	se.AddString(key, fmt.Sprintf("%+v", value))
}

func (se *setraceEncoder) AddFloat64(key string, value float64) {
	se.AddString(key, strconv.FormatFloat(value, 'f', 1, 64))
}

func (se *setraceEncoder) AddInt64(key string, value int64) {
	se.AddString(key, strconv.FormatInt(value, 10))
}

func (se *setraceEncoder) AddUint64(key string, value uint64) {
	se.AddString(key, strconv.FormatUint(value, 10))
}

func (se *setraceEncoder) AddTime(key string, value time.Time) {
	se.AddString(key, value.String())
}

func (se *setraceEncoder) AddBinary(key string, value []byte)     { se.AddString(key, string(value)) }
func (se *setraceEncoder) AddByteString(key string, value []byte) { se.AddString(key, string(value)) }
func (se *setraceEncoder) AddBool(key string, value bool) {
	se.AddString(key, strconv.FormatBool(value))
}
func (se *setraceEncoder) AddComplex64(key string, value complex64) {
	se.AddComplex128(key, complex128(value))
}
func (se *setraceEncoder) AddFloat32(key string, value float32) { se.AddFloat64(key, float64(value)) }
func (se *setraceEncoder) AddInt(key string, value int)         { se.AddInt64(key, int64(value)) }
func (se *setraceEncoder) AddInt32(key string, value int32)     { se.AddInt64(key, int64(value)) }
func (se *setraceEncoder) AddInt16(key string, value int16)     { se.AddInt64(key, int64(value)) }
func (se *setraceEncoder) AddInt8(key string, value int8)       { se.AddInt64(key, int64(value)) }
func (se *setraceEncoder) AddUint(key string, value uint)       { se.AddUint64(key, uint64(value)) }
func (se *setraceEncoder) AddUint32(key string, value uint32)   { se.AddUint64(key, uint64(value)) }
func (se *setraceEncoder) AddUint16(key string, value uint16)   { se.AddUint64(key, uint64(value)) }
func (se *setraceEncoder) AddUint8(key string, value uint8)     { se.AddUint64(key, uint64(value)) }
func (se *setraceEncoder) AddUintptr(key string, value uintptr) { se.AddUint64(key, uint64(value)) }

func (se *setraceEncoder) Clone() zapcore.Encoder {
	return se.clone()
}
func (se *setraceEncoder) clone() *setraceEncoder {
	fieldsCopy := make(map[string]string, len(se.fields))
	for k, v := range se.fields {
		fieldsCopy[k] = v
	}
	reflectFieldsCopy := make(map[string]json.RawMessage, len(se.reflectFields))
	for k, v := range se.reflectFields {
		reflectFieldsCopy[k] = v
	}
	return &setraceEncoder{
		serviceName:   se.serviceName,
		hostName:      se.hostName,
		cfg:           se.cfg,
		pool:          se.pool,
		namespace:     se.namespace,
		fields:        fieldsCopy,
		reflectFields: reflectFieldsCopy,
	}
}

func (se *setraceEncoder) EncodeEntry(entry zapcore.Entry, fields []zapcore.Field) (*buffer.Buffer, error) {
	// clone to get final logger
	final := se.clone()
	buf := final.pool.Get()

	// we pass on setrace metadata and environment, but we add all other keys
	var (
		meta  setraceMeta
		found bool
	)
	for _, field := range fields {
		if field.Key == setraceEnvironmentKey {
			continue
		}
		if field.Key == setraceMetaKey {
			meta = field.Interface.(setraceMeta)
			found = true
			continue
		}
		field.AddTo(final)
	}
	if !found {
		return buf, xerrors.Errorf("setrace meta not found while encoding entry %q, stack: %v", entry.Message, entry.Stack)
	}
	zap.String(setraceEnvironmentKey, se.environment).AddTo(final)

	logProto, err := newSetraceLogProto(entry, meta, se.serviceName, se.hostName, final.fields, final.reflectFields)
	if err != nil {
		return nil, xerrors.Errorf("log creation error: %w", err)
	}

	bytes, err := proto.Marshal(logProto)
	if err != nil {
		return nil, xerrors.Errorf("log proto marshal error: %w", err)
	}

	if err := binary.Write(buf, binary.LittleEndian, uint32(len(bytes))); err != nil {
		return nil, err
	}

	if _, err = buf.Write(bytes); err != nil {
		return nil, err
	}

	if _, err = buf.Write(ProtoseqMagicSequence[:]); err != nil {
		return nil, err
	}
	return buf, nil
}

func newSetraceLogProto(entry zapcore.Entry, meta setraceMeta, serviceName, hostName string, fields map[string]string, reflectFields map[string]json.RawMessage) (*setraceproto.TScenarioLog, error) {
	instanceDescriptor := &rtlog.InstanceDescriptor{
		ServiceName: ptr.String(serviceName),
		HostName:    ptr.String(hostName),
	}
	tScenarioLog := &setraceproto.TScenarioLog{
		ReqId:              ptr.String(meta.MainMeta.RequestID),
		ReqTimestamp:       ptr.Int64(meta.MainMeta.RequestTimestamp),
		ActivationId:       ptr.String(meta.MainMeta.ActivationID),
		FrameId:            ptr.Uint64(tools.HuidifyString(meta.MainMeta.RequestID)),
		EventIndex:         ptr.Uint64(meta.MainMeta.EventIndex),
		InstanceDescriptor: instanceDescriptor,
		Timestamp:          ptr.Int64(timestamp.FromTime(entry.Time).UnixMicro()),
	}
	switch meta.EventType {
	case activationStarted:
		message := &setraceproto.TScenarioLog_ActivationStarted{
			ActivationStarted: &rtlog.ActivationStarted{
				Instance:     &rtlog.ActivationStarted_InstanceDescriptor{InstanceDescriptor: instanceDescriptor},
				ReqTimestamp: ptr.Uint64(uint64(meta.MainMeta.RequestTimestamp)),
				ReqId:        ptr.String(meta.MainMeta.RequestID),
				ActivationId: ptr.String(meta.MainMeta.ActivationID),
				Pid:          ptr.Uint32(meta.MainMeta.Pid),
				Session:      ptr.Bool(false), // todo: check correct default value for this and Continue
				Continue:     ptr.Bool(false),
			},
		}
		tScenarioLog.Message = message
	case activationFinished:
		message := &setraceproto.TScenarioLog_ActivationFinished{
			ActivationFinished: &rtlog.ActivationFinished{},
		}
		tScenarioLog.Message = message
	case childActivationStarted:
		message := &setraceproto.TScenarioLog_ChildActivationStarted{
			ChildActivationStarted: &rtlog.ChildActivationStarted{
				ReqTimestamp: ptr.Uint64(uint64(meta.ChildActivationStartedMeta.RequestTimestamp)),
				ReqId:        ptr.String(meta.ChildActivationStartedMeta.RequestID),
				ActivationId: ptr.String(meta.ChildActivationStartedMeta.ChildActivationID),
				Description:  ptr.String(meta.ChildActivationStartedMeta.ChildDescription),
			},
		}
		tScenarioLog.Message = message
	case childActivationFinished:
		message := &setraceproto.TScenarioLog_ChildActivationFinished{
			ChildActivationFinished: &rtlog.ChildActivationFinished{
				ActivationId: ptr.String(meta.ChildActivationFinishedMeta.ChildActivationID),
				Ok:           ptr.Bool(meta.ChildActivationFinishedMeta.Success),
			},
		}
		tScenarioLog.Message = message
	case logEvent:
		messageFields := make(map[string]string, len(fields)+len(reflectFields))
		for key, value := range fields {
			messageFields[key] = value
		}
		for key, value := range reflectFields {
			messageFields[key] = string(value)
		}
		message := &setraceproto.TScenarioLog_LogEvent{
			LogEvent: &rtlog.LogEvent{
				Severity:  meta.LogEventMeta.severity(),
				Message:   ptr.String(entry.Message),
				Backtrace: ptr.String(meta.LogEventMeta.Backtrace),
				Fields:    messageFields,
			},
		}
		tScenarioLog.Message = message
	default:
		return nil, xerrors.Errorf("unknown meta event type: %v", meta.EventType)
	}
	return tScenarioLog, nil
}
