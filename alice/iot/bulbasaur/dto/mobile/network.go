package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type NetworkSaveRequest struct {
	SSID     string `json:"ssid"`
	Password string `json:"password"`
}

func (nsr NetworkSaveRequest) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	if len(nsr.SSID) == 0 {
		verrs = append(verrs, xerrors.New("ssid should not be empty"))
	}
	// according to standard - maximum length is 32 bytes
	if len([]byte(nsr.SSID)) > 64 {
		verrs = append(verrs, xerrors.New("ssid should not be longer than 64 bytes"))
	}
	if len(nsr.Password) == 0 {
		verrs = append(verrs, xerrors.New("password should not be empty"))
	}
	// according to WPA2 standard - maximum length is 64 bytes
	if len([]byte(nsr.Password)) > 128 {
		verrs = append(verrs, xerrors.New("password should not be longer than 64 bytes"))
	}
	if len(verrs) == 0 {
		return false, nil
	}
	return false, verrs
}

func (nsr *NetworkSaveRequest) ToNetwork() model.Network {
	network := model.Network{
		SSID:     nsr.SSID,
		Password: nsr.Password,
	}
	return network
}

type NetworkGetPasswordRequest struct {
	SSID string `json:"ssid"`
}

type NetworkGetPasswordResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	Password  string `json:"password"`
}

type NetworkDeleteRequest struct {
	SSID string `json:"ssid"`
}

type NetworkUseRequest struct {
	SSID string `json:"ssid"`
}
