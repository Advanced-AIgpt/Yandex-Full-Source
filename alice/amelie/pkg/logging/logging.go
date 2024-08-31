package logging

import (
	"context"
	"encoding/json"
	stdLog "log"
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

type loggerKey struct{}
type serviceLoggerKey struct{}

func getLogger(ctx context.Context, key interface{}) log.Logger {
	if logger, ok := ctx.Value(key).(log.Logger); ok {
		return logger
	}
	return new(nop.Logger)
}

func withLogger(ctx context.Context, key interface{}, logger log.Logger) context.Context {
	return context.WithValue(ctx, key, logger)
}

func Logger(ctx context.Context) log.Logger {
	return getLogger(ctx, loggerKey{})
}

func WithLogger(ctx context.Context, logger log.Logger) context.Context {
	return withLogger(ctx, loggerKey{}, logger)
}

func ServiceLogger(ctx context.Context) log.Logger {
	return getLogger(ctx, serviceLoggerKey{})
}

func WithServiceLogger(ctx context.Context, logger log.Logger) context.Context {
	return withLogger(ctx, serviceLoggerKey{}, logger)
}

const (
	HiddenPlaceholder = "***HIDDEN***"
)

func hideFields(v reflect.Value) {
	t := v.Type()
	for i := 0; i < v.NumField(); i++ {
		fv := v.Field(i)
		ft := t.Field(i)
		pp, ok := ft.Tag.Lookup("logging")
		if !ok {
			if fv.Kind() == reflect.Struct {
				hideFields(fv)
			}
			continue
		}
		props := strings.Split(pp, ",")
		for _, prop := range props {
			switch prop {
			case "hidden":
				fv.Set(reflect.Zero(fv.Type()))
			default:
				stdLog.Printf("unknown logging prop: %s", prop)
			}
		}
	}
}

type loggingOptions struct {
	Hidden bool
}

func parseLoggingOptions(tagValue string) loggingOptions {
	var opts loggingOptions
	for _, opt := range strings.Split(tagValue, ",") {
		switch opt {
		case "hidden":
			opts.Hidden = true
		}
	}
	return opts
}

func applyLoggingOptions(v reflect.Value, lv map[string]interface{}) {
loop:
	for {
		switch v.Kind() {
		case reflect.Ptr, reflect.Interface:
			v = v.Elem()
		case reflect.Struct:
			break loop
		default:
			return
		}
	}

	for i := 0; i < v.NumField(); i++ {
		fieldType := v.Type().Field(i)
		fieldValue := v.Field(i)
		fieldName := fieldType.Name
		if fieldType.Anonymous {
			applyLoggingOptions(fieldValue, lv)
			continue
		}
		if tag, ok := fieldType.Tag.Lookup("json"); ok {
			fieldName = strings.Split(tag, ",")[0]
		}
		subLv, ok := lv[fieldName]
		if !ok {
			continue
		}
		var opts loggingOptions
		if tag, ok := fieldType.Tag.Lookup("logging"); ok {
			opts = parseLoggingOptions(tag)
		}
		if opts.Hidden {
			lv[fieldName] = HiddenPlaceholder
			continue
		}
		if lvv, ok := subLv.(map[string]interface{}); ok {
			applyLoggingOptions(fieldValue, lvv)
			continue
		}
	}
}

func Marshal(origin interface{}) (map[string]interface{}, error) {
	buf, err := json.Marshal(origin)
	if err != nil {
		return nil, err
	}
	lv := map[string]interface{}{}
	if err := json.Unmarshal(buf, &lv); err != nil {
		return nil, err
	}
	v := reflect.ValueOf(origin)
	applyLoggingOptions(v, lv)
	return lv, nil
}

func MustMarshal(origin interface{}) map[string]interface{} {
	v, err := Marshal(origin)
	if err != nil {
		stdLog.Println(err)
	}
	return v
}
