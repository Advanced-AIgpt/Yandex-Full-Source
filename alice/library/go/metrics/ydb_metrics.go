package metrics

import (
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type YDBSignals struct {
	//general
	success           metrics.Counter
	ydbFails          metrics.Counter
	totalFails        metrics.Counter
	total             metrics.Counter
	durationHistogram metrics.Timer

	// op errors
	opErrorUnknownStatus      metrics.Counter
	opErrorBadRequest         metrics.Counter
	opErrorUnauthorized       metrics.Counter
	opErrorInternalError      metrics.Counter
	opErrorAborted            metrics.Counter
	opErrorUnavailable        metrics.Counter
	opErrorOverloaded         metrics.Counter
	opErrorSchemeError        metrics.Counter
	opErrorGenericError       metrics.Counter
	opErrorTimeout            metrics.Counter
	opErrorBadSession         metrics.Counter
	opErrorPreconditionFailed metrics.Counter
	opErrorAlreadyExists      metrics.Counter
	opErrorNotFound           metrics.Counter
	opErrorSessionExpired     metrics.Counter
	opErrorCancelled          metrics.Counter
	opErrorUndetermined       metrics.Counter
	opErrorUnsupported        metrics.Counter
	opErrorSessionBusy        metrics.Counter
	opErrorOther              metrics.Counter

	// transport errors
	transportErrorCanceled           metrics.Counter
	transportErrorUnknown            metrics.Counter
	transportErrorInvalidArgument    metrics.Counter
	transportErrorDeadlineExceeded   metrics.Counter
	transportErrorNotFound           metrics.Counter
	transportErrorAlreadyExists      metrics.Counter
	transportErrorPermissionDenied   metrics.Counter
	transportErrorResourceExhausted  metrics.Counter
	transportErrorFailedPrecondition metrics.Counter
	transportErrorAborted            metrics.Counter
	transportErrorOutOfRange         metrics.Counter
	transportErrorUnimplemented      metrics.Counter
	transportErrorInternal           metrics.Counter
	transportErrorUnavailable        metrics.Counter
	transportErrorDataLoss           metrics.Counter
	transportErrorUnauthenticated    metrics.Counter
	transportErrorOther              metrics.Counter

	// other error
	otherError metrics.Counter
}

func NewYDBSignals(method string, registry metrics.Registry, policy BucketsGenerationPolicy) YDBSignals {
	s := YDBSignals{
		success:           registry.WithTags(map[string]string{"db_method": method}).Counter("success"),
		ydbFails:          registry.WithTags(map[string]string{"db_method": method, "error_type": "ydb_total"}).Counter("fails"),
		totalFails:        registry.WithTags(map[string]string{"db_method": method, "error_type": "total"}).Counter("fails"),
		total:             registry.WithTags(map[string]string{"db_method": method}).Counter("total"),
		durationHistogram: registry.WithTags(map[string]string{"db_method": method}).DurationHistogram("duration_buckets", policy()),

		opErrorUnknownStatus:      registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "UnknownStatus"}).Counter("fails"),
		opErrorBadRequest:         registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "BadRequest"}).Counter("fails"),
		opErrorUnauthorized:       registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Unauthorized"}).Counter("fails"),
		opErrorInternalError:      registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "InternalError"}).Counter("fails"),
		opErrorAborted:            registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Aborted"}).Counter("fails"),
		opErrorUnavailable:        registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Unavailable"}).Counter("fails"),
		opErrorOverloaded:         registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Overloaded"}).Counter("fails"),
		opErrorSchemeError:        registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "SchemeError"}).Counter("fails"),
		opErrorGenericError:       registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "GenericError"}).Counter("fails"),
		opErrorTimeout:            registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Timeout"}).Counter("fails"),
		opErrorBadSession:         registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "BadSession"}).Counter("fails"),
		opErrorPreconditionFailed: registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "PreconditionFailed"}).Counter("fails"),
		opErrorAlreadyExists:      registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "AlreadyExists"}).Counter("fails"),
		opErrorNotFound:           registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "NotFound"}).Counter("fails"),
		opErrorSessionExpired:     registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "SessionExpired"}).Counter("fails"),
		opErrorCancelled:          registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Cancelled"}).Counter("fails"),
		opErrorUndetermined:       registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Undetermined"}).Counter("fails"),
		opErrorUnsupported:        registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Unsupported"}).Counter("fails"),
		opErrorSessionBusy:        registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "SessionBusy"}).Counter("fails"),
		opErrorOther:              registry.WithTags(map[string]string{"db_method": method, "error_type": "operation", "reason": "Other"}).Counter("fails"),

		// transport errors
		transportErrorCanceled:           registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Canceled"}).Counter("fails"),
		transportErrorUnknown:            registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Unknown"}).Counter("fails"),
		transportErrorInvalidArgument:    registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "InvalidArgument"}).Counter("fails"),
		transportErrorDeadlineExceeded:   registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "DeadlineExceeded"}).Counter("fails"),
		transportErrorNotFound:           registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "NotFound"}).Counter("fails"),
		transportErrorAlreadyExists:      registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "AlreadyExists"}).Counter("fails"),
		transportErrorPermissionDenied:   registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "PermissionDenied"}).Counter("fails"),
		transportErrorResourceExhausted:  registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "ResourceExhausted"}).Counter("fails"),
		transportErrorFailedPrecondition: registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "FailedPrecondition"}).Counter("fails"),
		transportErrorAborted:            registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Aborted"}).Counter("fails"),
		transportErrorOutOfRange:         registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "OutOfRange"}).Counter("fails"),
		transportErrorUnimplemented:      registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Unimplemented"}).Counter("fails"),
		transportErrorInternal:           registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Internal"}).Counter("fails"),
		transportErrorUnavailable:        registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Unavailable"}).Counter("fails"),
		transportErrorDataLoss:           registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "DataLoss"}).Counter("fails"),
		transportErrorUnauthenticated:    registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Unauthenticated"}).Counter("fails"),
		transportErrorOther:              registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "Other"}).Counter("fails"),

		// other error
		otherError: registry.WithTags(map[string]string{"db_method": method, "error_type": "other"}).Counter("fails"),
	}

	solomon.Rated(s.success)
	solomon.Rated(s.ydbFails)
	solomon.Rated(s.totalFails)
	solomon.Rated(s.total)
	solomon.Rated(s.durationHistogram)

	solomon.Rated(s.opErrorUnknownStatus)
	solomon.Rated(s.opErrorBadRequest)
	solomon.Rated(s.opErrorUnauthorized)
	solomon.Rated(s.opErrorInternalError)
	solomon.Rated(s.opErrorAborted)
	solomon.Rated(s.opErrorUnavailable)
	solomon.Rated(s.opErrorOverloaded)
	solomon.Rated(s.opErrorSchemeError)
	solomon.Rated(s.opErrorGenericError)
	solomon.Rated(s.opErrorTimeout)
	solomon.Rated(s.opErrorBadSession)
	solomon.Rated(s.opErrorPreconditionFailed)
	solomon.Rated(s.opErrorAlreadyExists)
	solomon.Rated(s.opErrorNotFound)
	solomon.Rated(s.opErrorSessionExpired)
	solomon.Rated(s.opErrorCancelled)
	solomon.Rated(s.opErrorUndetermined)
	solomon.Rated(s.opErrorUnsupported)
	solomon.Rated(s.opErrorSessionBusy)
	solomon.Rated(s.opErrorOther)

	solomon.Rated(s.transportErrorCanceled)
	solomon.Rated(s.transportErrorUnknown)
	solomon.Rated(s.transportErrorInvalidArgument)
	solomon.Rated(s.transportErrorDeadlineExceeded)
	solomon.Rated(s.transportErrorNotFound)
	solomon.Rated(s.transportErrorAlreadyExists)
	solomon.Rated(s.transportErrorPermissionDenied)
	solomon.Rated(s.transportErrorResourceExhausted)
	solomon.Rated(s.transportErrorFailedPrecondition)
	solomon.Rated(s.transportErrorAborted)
	solomon.Rated(s.transportErrorOutOfRange)
	solomon.Rated(s.transportErrorUnimplemented)
	solomon.Rated(s.transportErrorInternal)
	solomon.Rated(s.transportErrorUnavailable)
	solomon.Rated(s.transportErrorDataLoss)
	solomon.Rated(s.transportErrorUnauthenticated)
	solomon.Rated(s.transportErrorOther)

	solomon.Rated(s.otherError)
	return s
}

func (s *YDBSignals) RecordMetrics(err error) {
	s.total.Inc()
	if err == nil {
		s.success.Inc()
		return
	}
	s.totalFails.Inc()

	var (
		transportErr *ydb.TransportError
		opErr        *ydb.OpError
	)
	switch {
	case xerrors.As(err, &opErr):
		s.ydbFails.Inc()
		switch opErr.Reason {
		case ydb.StatusBadRequest:
			s.opErrorBadRequest.Inc()
		case ydb.StatusUnauthorized:
			s.opErrorUnauthorized.Inc()
		case ydb.StatusInternalError:
			s.opErrorInternalError.Inc()
		case ydb.StatusAborted:
			s.opErrorAborted.Inc()
		case ydb.StatusUnavailable:
			s.opErrorUnavailable.Inc()
		case ydb.StatusOverloaded:
			s.opErrorOverloaded.Inc()
		case ydb.StatusSchemeError:
			s.opErrorSchemeError.Inc()
		case ydb.StatusGenericError:
			s.opErrorGenericError.Inc()
		case ydb.StatusTimeout:
			s.opErrorTimeout.Inc()
		case ydb.StatusBadSession:
			s.opErrorBadSession.Inc()
		case ydb.StatusPreconditionFailed:
			s.opErrorPreconditionFailed.Inc()
		case ydb.StatusAlreadyExists:
			s.opErrorAlreadyExists.Inc()
		case ydb.StatusNotFound:
			s.opErrorNotFound.Inc()
		case ydb.StatusSessionExpired:
			s.opErrorSessionExpired.Inc()
		case ydb.StatusCancelled:
			s.opErrorCancelled.Inc()
		case ydb.StatusUndetermined:
			s.opErrorUndetermined.Inc()
		case ydb.StatusUnsupported:
			s.opErrorUnsupported.Inc()
		case ydb.StatusSessionBusy:
			s.opErrorSessionBusy.Inc()
		default:
			s.opErrorOther.Inc()
		}
	case xerrors.As(err, &transportErr):
		s.ydbFails.Inc()
		switch transportErr.Reason {
		case ydb.TransportErrorCanceled:
			s.transportErrorCanceled.Inc()
		case ydb.TransportErrorUnknown:
			s.transportErrorUnknown.Inc()
		case ydb.TransportErrorInvalidArgument:
			s.transportErrorInvalidArgument.Inc()
		case ydb.TransportErrorDeadlineExceeded:
			s.transportErrorDeadlineExceeded.Inc()
		case ydb.TransportErrorNotFound:
			s.transportErrorNotFound.Inc()
		case ydb.TransportErrorAlreadyExists:
			s.transportErrorAlreadyExists.Inc()
		case ydb.TransportErrorPermissionDenied:
			s.transportErrorPermissionDenied.Inc()
		case ydb.TransportErrorResourceExhausted:
			s.transportErrorResourceExhausted.Inc()
		case ydb.TransportErrorFailedPrecondition:
			s.transportErrorFailedPrecondition.Inc()
		case ydb.TransportErrorAborted:
			s.transportErrorAborted.Inc()
		case ydb.TransportErrorOutOfRange:
			s.transportErrorOutOfRange.Inc()
		case ydb.TransportErrorUnimplemented:
			s.transportErrorUnimplemented.Inc()
		case ydb.TransportErrorInternal:
			s.transportErrorInternal.Inc()
		case ydb.TransportErrorUnavailable:
			s.transportErrorUnavailable.Inc()
		case ydb.TransportErrorDataLoss:
			s.transportErrorDataLoss.Inc()
		case ydb.TransportErrorUnauthenticated:
			s.transportErrorUnauthenticated.Inc()
		default:
			s.transportErrorOther.Inc()
		}
	default:
		s.otherError.Inc()
	}
}

func (s *YDBSignals) RecordDurationSince(start time.Time) {
	s.durationHistogram.RecordDuration(time.Since(start))
}
