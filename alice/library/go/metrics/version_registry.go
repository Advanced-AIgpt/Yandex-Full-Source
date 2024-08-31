package metrics

import (
	"strconv"

	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

func NewVersionRegistry(logger log.Logger, registry *solomon.Registry) *solomon.Registry {
	svnRevision, err := strconv.Atoi(buildinfo.Info.SVNRevision)
	if err != nil {
		logger.Warnf("can't convert svn revision %s to number: %v", buildinfo.Info.SVNRevision, err)
		svnRevision = -1
	}
	registry.WithPrefix("binary").Gauge("version").Set(float64(svnRevision))
	return registry
}
