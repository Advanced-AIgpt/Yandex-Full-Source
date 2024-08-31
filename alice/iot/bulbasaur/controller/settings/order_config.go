package settings

import (
	memento "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/alice/protos/data/scenario/order"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"google.golang.org/protobuf/types/known/anypb"
)

type OrderStatusConfig struct {
	*order.TOrderStatusUserConfig
}

func NewOrderStatusConfig() *OrderStatusConfig {
	return &OrderStatusConfig{TOrderStatusUserConfig: &order.TOrderStatusUserConfig{}}
}

func (c *OrderStatusConfig) From(configPair *memento.TConfigKeyAnyPair) error {
	if configPair.Key != c.Key() {
		return xerrors.Errorf("invalid config key to build order status config from: %s", configPair.Key.String())
	}
	var orderStatusConfig order.TOrderStatusUserConfig
	if err := configPair.Value.UnmarshalTo(&orderStatusConfig); err != nil {
		return xerrors.Errorf("cannot unmarshal order status config: %v", err)
	}
	c.TOrderStatusUserConfig = &orderStatusConfig
	return nil
}

func (c *OrderStatusConfig) ExtractFrom(settings UserSettings) (bool, error) {
	if settings.Order == nil {
		return false, nil
	}
	if settings.Order.HideItemNames == nil {
		return false, nil
	}
	c.TOrderStatusUserConfig.HideItemNames = *settings.Order.HideItemNames
	return true, nil
}

func (c OrderStatusConfig) ExtractTo(settings UserSettings) (UserSettings, error) {
	result := settings.Clone()
	result.Order = &OrderSettings{HideItemNames: ptr.Bool(c.TOrderStatusUserConfig.HideItemNames)}
	return *result, nil
}

func (c OrderStatusConfig) Build() (*memento.TConfigKeyAnyPair, error) {
	serialized, err := anypb.New(c.TOrderStatusUserConfig)
	if err != nil {
		return nil, xerrors.Errorf("failed to serialize order status config: %v", err)
	}
	return &memento.TConfigKeyAnyPair{
		Key:   c.Key(),
		Value: serialized,
	}, nil
}

func (c OrderStatusConfig) Key() memento.EConfigKey {
	return memento.EConfigKey_CK_ORDER_STATUS
}
