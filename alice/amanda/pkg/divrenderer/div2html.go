package divrenderer

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"time"

	"a.yandex-team.ru/alice/amanda/internal/uuid"
)

const (
	_div2HTMLURL = "http://div2html.s3.yandex.net/div2.html"
)

type DIVRenderer struct {
	viewer WEBViewer
	cloud  Cloud
}

type WEBViewer interface {
	ToPNG(url string) (image []byte, error error)
}

type Cloud interface {
	Share(key string) (url string, err error)
	Exists(key string) (bool, error)
	Delete(key string) error
	Upload(key string, content io.Reader) error
}

// New creates DIVRenderer with viewer and cloud, ensure that viewer has access to the shared link
func New(viewer WEBViewer, cloud Cloud) *DIVRenderer {
	return &DIVRenderer{
		viewer: viewer,
		cloud:  cloud,
	}
}

// Render returns a dialog as png image
func (r *DIVRenderer) Render(dialog *Dialog) (image []byte, err error) {
	body, err := json.Marshal(dialog.messages)
	if err != nil {
		return nil, fmt.Errorf("unable to parse dialogs: %w", err)
	}
	return r.RenderJSONDialog(body)
}

func (r *DIVRenderer) RenderJSONDialog(dialog []byte) (image []byte, err error) {
	key := "src/amanda/" + time.Now().Format("2006-01-02-15-04-05") + uuid.New() + ".json"
	if err = r.cloud.Upload(key, bytes.NewReader(dialog)); err != nil {
		return nil, fmt.Errorf("unable to upload dialog with the key %s: %w", key, err)
	}
	defer func() {
		_ = r.cloud.Delete(key)
	}()
	link, err := r.cloud.Share(key)
	if err != nil {
		return nil, fmt.Errorf("unable to share dialog with key %s: %w", key, err)
	}
	image, err = r.viewer.ToPNG(makeDIV2HTMLLink(link))
	if err != nil {
		return nil, fmt.Errorf("unable to render url %s: %w", link, err)
	}
	return image, nil
}

func makeDIV2HTMLLink(dialogURL string) string {
	return _div2HTMLURL + "?url=" + dialogURL
}
