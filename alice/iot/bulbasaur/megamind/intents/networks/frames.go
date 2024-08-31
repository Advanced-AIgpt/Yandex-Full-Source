package networks

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type RestoreNetworksFrame struct{}

func (r *RestoreNetworksFrame) FromTypedSemanticFrame(_ *common.TTypedSemanticFrame) error {
	return nil
}

func (r *RestoreNetworksFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_RestoreIotNetworksSemanticFrame{
			RestoreIotNetworksSemanticFrame: &common.TRestoreIotNetworksSemanticFrame{},
		},
	}
}

type SaveNetworksFrame struct {
	Networks model.SpeakerNetworks `json:"networks"`
}

func (s *SaveNetworksFrame) FromTypedSemanticFrame(p *common.TTypedSemanticFrame) error {
	s.Networks.FromProto(p.GetSaveIotNetworksSemanticFrame().GetNetworks().GetNetworksValue())
	return nil
}

func (s *SaveNetworksFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_SaveIotNetworksSemanticFrame{
			SaveIotNetworksSemanticFrame: &common.TSaveIotNetworksSemanticFrame{
				Networks: &common.TIotNetworksSlot{
					Value: &common.TIotNetworksSlot_NetworksValue{
						NetworksValue: s.Networks.ToProto(),
					},
				},
			},
		},
	}
}

type DeleteNetworksFrame struct{}

func (d *DeleteNetworksFrame) FromTypedSemanticFrame(p *common.TTypedSemanticFrame) error {
	return nil
}

func (d *DeleteNetworksFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_DeleteIotNetworksSemanticFrame{
			DeleteIotNetworksSemanticFrame: &common.TDeleteIotNetworksSemanticFrame{},
		},
	}
}
