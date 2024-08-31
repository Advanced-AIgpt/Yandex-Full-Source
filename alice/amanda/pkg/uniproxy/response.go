package uniproxy

import (
	"a.yandex-team.ru/alice/amanda/pkg/speechkit"
)

type Response struct {
	SKResponse  *speechkit.Response
	TTSResponse *speechkit.TTSResponse
	ASRText     *string
}
