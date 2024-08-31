package dialogs

type HostURL string

const (
	ProductionURL HostURL = "paskills.voicetech.yandex.net"
	TestingURL    HostURL = "paskills.test.voicetech.yandex.net"
	PriemkaURL    HostURL = "paskills.priemka.voicetech.yandex.net"
)

type HostTVMAlias string

const (
	// aliases from iot/bulbasaur/misc/tvm.sh
	ProductionTVMAlias HostTVMAlias = "dialogs"
	TestingTVMAlias    HostTVMAlias = "dialogs-testing"
	PriemkaTVMAlias    HostTVMAlias = "dialogs-priemka"
)

var KnownDialogsHosts map[HostURL]HostTVMAlias

func init() {
	KnownDialogsHosts = map[HostURL]HostTVMAlias{
		ProductionURL: ProductionTVMAlias,
		TestingURL:    TestingTVMAlias,
		PriemkaURL:    PriemkaTVMAlias,
	}
}

type ServiceKey string

const (
	DialogsServiceKey ServiceKey = "SKILLS_HOST"
)

type HostInfo struct {
	URL      HostURL
	TVMAlias HostTVMAlias
}
