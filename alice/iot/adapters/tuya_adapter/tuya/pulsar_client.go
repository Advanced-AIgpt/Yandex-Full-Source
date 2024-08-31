package tuya

import (
	"context"
	"crypto/md5"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"strconv"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/library/go/core/xerrors"
	pulsar "github.com/TuyaInc/tuya_pulsar_sdk_go"
	"github.com/TuyaInc/tuya_pulsar_sdk_go/pkg/tyutils"
)

func HandlePulsarEvents(ctx context.Context, logger log.Logger, addr, accessID, accessKey string, handler PulsarHandler) error {
	// create client
	client := pulsar.NewClient(pulsar.ClientConfig{
		PulsarAddr: addr,
	})

	// create consumer
	csm, err := client.NewConsumer(pulsar.ConsumerConfig{
		Topic: pulsar.TopicForAccessID(accessID),
		Auth:  pulsar.NewAuthProvider(accessID, accessKey),
	})
	if err != nil {
		return err
	}

	// handle message
	go csm.ReceiveAndHandle(ctx, &pulsarMessageDecoder{logger: logger, secret: accessKey, handler: handler})
	return nil
}

type PulsarHandler func(ctx context.Context, msgID string, timestamp int64, protocol int, payload []byte) error

type pulsarMessageDecoder struct {
	logger  log.Logger
	secret  string
	handler PulsarHandler
}

func (d *pulsarMessageDecoder) HandlePayload(ctx context.Context, msg *pulsar.Message, payload []byte) error {
	msgID := msg.Msg.MessageId.String()
	ctx = ctxlog.WithFields(ctx, log.String("pulsar_message_id", msgID))

	message := struct {
		Protocol int    `json:"protocol"`
		PV       string `json:"pv"`
		TS       int64  `json:"t"`
		Data     string `json:"data"`
		Sign     string `json:"sign"`
	}{}
	if err := json.Unmarshal(payload, &message); err != nil {
		ctxlog.Infof(ctx, d.logger, "got raw payload from pulsar: %s", payload)
		ctxlog.Warnf(ctx, d.logger, "json unmarshal failed: %v", err)
		return err
	}

	ctx = ctxlog.WithFields(ctx, log.Int("pulsar_message_protocol", message.Protocol))

	// check payload sign
	assembly := make([]byte, 0)
	assembly = append(assembly, []byte("data="+message.Data+"||")...)
	assembly = append(assembly, []byte("protocol="+strconv.Itoa(message.Protocol)+"||")...)
	assembly = append(assembly, []byte("pv="+message.PV+"||")...)
	assembly = append(assembly, []byte("t="+strconv.FormatInt(message.TS, 10)+"||")...)
	assembly = append(assembly, []byte(d.secret)...)
	hash := md5.Sum(assembly)

	if hex.EncodeToString(hash[:]) != message.Sign {
		ctxlog.Infof(ctx, d.logger, "got raw payload from pulsar: %s", payload)
		ctxlog.Warnf(ctx, d.logger, "message sign mismatch: expected %q, got %q", hex.EncodeToString(hash[:]), message.Sign)
		return xerrors.New("wrong message sign")
	}

	// decode the payload with AES ECB
	bytes, err := base64.StdEncoding.DecodeString(message.Data)
	if err != nil {
		ctxlog.Infof(ctx, d.logger, "got raw payload from pulsar: %s", payload)
		ctxlog.Warnf(ctx, d.logger, "base64 decode failed: %v", err)
		return err
	}
	decodedData := tyutils.EcbDecrypt(bytes, []byte(d.secret[8:24]))

	return d.handler(ctx, msgID, message.TS, message.Protocol, decodedData)
}
