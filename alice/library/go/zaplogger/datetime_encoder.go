package zaplogger

import (
	"time"

	"go.uber.org/zap"
	"go.uber.org/zap/buffer"
	"go.uber.org/zap/zapcore"
)

type DatetimeEncoderWrapper struct {
	inner zapcore.Encoder
}

func NewDatetimeEncoderWrapper(formatEncoder zapcore.Encoder) zapcore.Encoder {
	return &DatetimeEncoderWrapper{inner: formatEncoder}
}

const timeLayout = "2006-01-02T15:04:05.000"

func (d *DatetimeEncoderWrapper) EncodeEntry(entry zapcore.Entry, fields []zapcore.Field) (*buffer.Buffer, error) {
	fields = append(fields, zap.String("dt", entry.Time.Format(timeLayout)))
	return d.inner.EncodeEntry(entry, fields)
}
func (d *DatetimeEncoderWrapper) AddArray(k string, m zapcore.ArrayMarshaler) error {
	return d.inner.AddArray(k, m)
}
func (d *DatetimeEncoderWrapper) AddObject(k string, m zapcore.ObjectMarshaler) error {
	return d.inner.AddObject(k, m)
}
func (d *DatetimeEncoderWrapper) AddReflected(k string, v interface{}) error {
	return d.inner.AddReflected(k, v)
}
func (d *DatetimeEncoderWrapper) AddBinary(k string, v []byte)          { d.inner.AddBinary(k, v) }
func (d *DatetimeEncoderWrapper) AddByteString(k string, v []byte)      { d.inner.AddByteString(k, v) }
func (d *DatetimeEncoderWrapper) AddBool(k string, v bool)              { d.inner.AddBool(k, v) }
func (d *DatetimeEncoderWrapper) AddComplex128(k string, v complex128)  { d.inner.AddComplex128(k, v) }
func (d *DatetimeEncoderWrapper) AddComplex64(k string, v complex64)    { d.inner.AddComplex64(k, v) }
func (d *DatetimeEncoderWrapper) AddDuration(k string, v time.Duration) { d.inner.AddDuration(k, v) }
func (d *DatetimeEncoderWrapper) AddFloat64(k string, v float64)        { d.inner.AddFloat64(k, v) }
func (d *DatetimeEncoderWrapper) AddFloat32(k string, v float32)        { d.inner.AddFloat32(k, v) }
func (d *DatetimeEncoderWrapper) AddInt(k string, v int)                { d.inner.AddInt(k, v) }
func (d *DatetimeEncoderWrapper) AddInt64(k string, v int64)            { d.inner.AddInt64(k, v) }
func (d *DatetimeEncoderWrapper) AddInt32(k string, v int32)            { d.inner.AddInt32(k, v) }
func (d *DatetimeEncoderWrapper) AddInt16(k string, v int16)            { d.inner.AddInt16(k, v) }
func (d *DatetimeEncoderWrapper) AddInt8(k string, v int8)              { d.inner.AddInt8(k, v) }
func (d *DatetimeEncoderWrapper) AddString(k, v string)                 { d.inner.AddString(k, v) }
func (d *DatetimeEncoderWrapper) AddTime(k string, v time.Time)         { d.inner.AddTime(k, v) }
func (d *DatetimeEncoderWrapper) AddUint(k string, v uint)              { d.inner.AddUint(k, v) }
func (d *DatetimeEncoderWrapper) AddUint64(k string, v uint64)          { d.inner.AddUint64(k, v) }
func (d *DatetimeEncoderWrapper) AddUint32(k string, v uint32)          { d.inner.AddUint32(k, v) }
func (d *DatetimeEncoderWrapper) AddUint16(k string, v uint16)          { d.inner.AddUint16(k, v) }
func (d *DatetimeEncoderWrapper) AddUint8(k string, v uint8)            { d.inner.AddUint8(k, v) }
func (d *DatetimeEncoderWrapper) AddUintptr(k string, v uintptr)        { d.inner.AddUintptr(k, v) }
func (d *DatetimeEncoderWrapper) OpenNamespace(k string)                { d.inner.OpenNamespace(k) }
func (d *DatetimeEncoderWrapper) Clone() zapcore.Encoder {
	return &DatetimeEncoderWrapper{d.inner.Clone()}
}
