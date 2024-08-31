package render

import (
	"context"
	"net/http"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/httputil/headers"
)

type ProtoRenderer struct {
	Logger log.Logger
}

func (p *ProtoRenderer) renderProto(ctx context.Context, w http.ResponseWriter, code int, message proto.Message) {
	protoPayload, err := proto.Marshal(message)
	if err != nil {
		ctxlog.Warn(ctx, p.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}
	ctxlog.Debugf(ctx, p.Logger, "Responding with: %s", proto.MarshalTextString(message))
	w.Header().Set(headers.ContentTypeKey, string(headers.TypeApplicationProtobuf))
	w.WriteHeader(code)
	_, _ = w.Write(protoPayload)
}

func (p *ProtoRenderer) RenderProto(ctx context.Context, w http.ResponseWriter, payload proto.Message) {
	p.renderProto(ctx, w, http.StatusOK, payload)
}
