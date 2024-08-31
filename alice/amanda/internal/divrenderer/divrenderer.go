package divrenderer

import (
	"a.yandex-team.ru/alice/amanda/pkg/divrenderer/mds"
	"a.yandex-team.ru/alice/amanda/pkg/divrenderer/rotor"
	"fmt"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"

	"a.yandex-team.ru/alice/amanda/internal/tvm"
	"a.yandex-team.ru/alice/amanda/pkg/divrenderer"
)

const (
	_s3MDSURL = "http://s3.mds.yandex.net/"
	_s3bucket = "div2html"
	_s3Region = "mds"
	_s3token  = ""
)

type Service interface {
	RenderDIVCard(body map[string]interface{}) (image []byte, err error)
}

type service struct {
	divRendererFactory func() (*divrenderer.DIVRenderer, error)
}

func New(source, s3accessKey, s3secretKey string, client tvm.Client) Service {
	return &service{
		divRendererFactory: func() (*divrenderer.DIVRenderer, error) {
			ticket, err := client.GetServiceTicket()
			if err != nil {
				return nil, err
			}
			return divrenderer.New(rotor.New(source, ticket), mds.New(
				aws.NewConfig().
					WithEndpoint(_s3MDSURL).
					WithDisableSSL(true).
					WithCredentials(credentials.NewStaticCredentials(s3accessKey, s3secretKey, _s3token)).
					WithRegion(_s3Region).
					WithMaxRetries(3),
				_s3bucket),
			), nil
		},
	}
}

func (s *service) RenderDIVCard(body map[string]interface{}) (data []byte, err error) {
	ch := make(chan struct {
		data []byte
		err  error
	})
	defer close(ch)
	div, err := s.divRendererFactory()
	if err != nil {
		return nil, err
	}
	for i := 0; i < 3; i++ {
		go func() {
			r, e := div.Render(divrenderer.NewDialog().AddAliceMessage(divrenderer.NewDivCard(body)))
			ch <- struct {
				data []byte
				err  error
			}{data: r, err: e}
		}()
	}
	for i := 0; i < 3; i++ {
		item := <-ch
		if len(item.data) > len(data) || (err != nil && item.err == nil) {
			data = item.data
			err = item.err
		}
		fmt.Printf(`divrenderer attempt %d: {"error": %v, "size": "%d"}`+"\n", i, item.err, len(item.data))
	}
	return data, err
}
