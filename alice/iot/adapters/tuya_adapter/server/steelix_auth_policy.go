package tuya

import (
	"a.yandex-team.ru/library/go/core/xerrors"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"github.com/go-resty/resty/v2"
)

type SteelixAuthPolicy struct {
	TVMAlias string
	Tvm      tvm.Client
}

func (ap *SteelixAuthPolicy) Apply(request *resty.Request) error {
	tvmServiceTicket, err := ap.Tvm.GetServiceTicketForAlias(request.Context(), ap.TVMAlias)
	if err != nil {
		return xerrors.Errorf("failed get TVM service ticket for steelix request: %w", err)
	}
	request.SetHeaders(map[string]string{
		tvmconsts.XYaServiceTicket: tvmServiceTicket,
	})
	return nil
}
