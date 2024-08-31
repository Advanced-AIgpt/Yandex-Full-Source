package takeout

import (
	"encoding/json"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ResponseView struct {
	Status string            `json:"status"`
	Error  string            `json:"error,omitempty"`
	Data   *ResponseDataView `json:"data,omitempty"`
}

type ResponseDataView struct {
	DataJSON string `json:"data.json,omitempty"`
}

func (r *ResponseView) FromData(data DataView) error {
	if data.IsEmpty() {
		r.Status = "no_data"
		r.Error = ""
		r.Data = nil
		return nil
	}

	dataJSON, err := json.Marshal(data)
	if err != nil {
		return xerrors.Errorf("Cannot marshal takeout data: %w", err)
	}

	r.Status = "ok"
	r.Error = ""
	r.Data = &ResponseDataView{
		DataJSON: string(dataJSON),
	}

	return nil
}
