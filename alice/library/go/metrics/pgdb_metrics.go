package metrics

import (
	"sync"
	"time"

	"github.com/jackc/pgconn"

	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type PGDBSignals struct {
	//general
	success           metrics.Counter
	pgdbFails         metrics.Counter
	totalFails        metrics.Counter
	total             metrics.Counter
	durationHistogram metrics.Timer

	// transport errors
	masterUnavailableError metrics.Counter
	nodeUnavailableError   metrics.Counter

	// operation errors
	opErrors map[string]metrics.Counter
	opMutex  *sync.Mutex

	// other error
	otherError metrics.Counter

	// method, registry and policy for opErrors generation
	method   string
	registry metrics.Registry
	policy   BucketsGenerationPolicy
}

func NewPGDBSignals(method string, registry metrics.Registry, policy BucketsGenerationPolicy) PGDBSignals {
	s := PGDBSignals{
		success:           registry.WithTags(map[string]string{"db_method": method}).Counter("success"),
		pgdbFails:         registry.WithTags(map[string]string{"db_method": method, "error_type": "pgdb_total"}).Counter("fails"),
		totalFails:        registry.WithTags(map[string]string{"db_method": method, "error_type": "total"}).Counter("fails"),
		total:             registry.WithTags(map[string]string{"db_method": method}).Counter("total"),
		durationHistogram: registry.WithTags(map[string]string{"db_method": method}).DurationHistogram("duration_buckets", policy()),

		// transport errors
		masterUnavailableError: registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "master_unavailable"}).Counter("fails"),
		nodeUnavailableError:   registry.WithTags(map[string]string{"db_method": method, "error_type": "transport", "reason": "node_unavailable"}).Counter("fails"),

		// operation errors
		opErrors: make(map[string]metrics.Counter),
		opMutex:  new(sync.Mutex),

		// other error
		otherError: registry.WithTags(map[string]string{"db_method": method, "error_type": "other"}).Counter("fails"),

		method:   method,
		registry: registry,
		policy:   policy,
	}

	solomon.Rated(s.success)
	solomon.Rated(s.pgdbFails)
	solomon.Rated(s.totalFails)
	solomon.Rated(s.total)
	solomon.Rated(s.durationHistogram)

	solomon.Rated(s.masterUnavailableError)
	solomon.Rated(s.nodeUnavailableError)

	solomon.Rated(s.otherError)
	return s
}

func (s *PGDBSignals) getOperationErrorCounter(code string) metrics.Counter {
	if opError, ok := s.opErrors[code]; ok {
		return opError
	} else {
		s.opMutex.Lock()
		defer s.opMutex.Unlock()
		if opError, ok := s.opErrors[code]; ok {
			return opError
		}
		opError := s.registry.WithTags(map[string]string{"db_method": s.method, "error_type": "operation", "reason": code}).Counter("fails")
		solomon.Rated(opError)
		s.opErrors[code] = opError
		return opError
	}
}

func (s *PGDBSignals) RecordMetrics(err error) {
	s.total.Inc()
	if err == nil {
		s.success.Inc()
		return
	}
	s.totalFails.Inc()

	var (
		pgxErr *pgconn.PgError
	)
	switch {
	case xerrors.Is(err, pgclient.ErrMasterIsUnavailable):
		s.pgdbFails.Inc()
		s.masterUnavailableError.Inc()
	case xerrors.Is(err, pgclient.ErrNodeIsUnavailable):
		s.pgdbFails.Inc()
		s.nodeUnavailableError.Inc()
	case xerrors.As(err, &pgxErr):
		s.pgdbFails.Inc()
		s.getOperationErrorCounter(pgxErr.Code).Inc()
	default:
		s.otherError.Inc()
	}
}

func (s *PGDBSignals) RecordDurationSince(start time.Time) {
	s.durationHistogram.RecordDuration(time.Since(start))
}
