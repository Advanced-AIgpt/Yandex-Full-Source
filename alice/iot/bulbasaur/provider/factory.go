package provider

import (
	"context"
	"fmt"
	"net/http"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/servicehost"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Factory struct {
	Socialism       socialism.IClient
	Dialogs         dialogs.Dialoger
	Bass            bass.IBass
	Notificator     notificator.IController
	Logger          log.Logger
	DefaultClient   *http.Client
	ZoraClient      *zora.Client
	Tvm             tvm.Client
	SignalsRegistry *SignalsRegistry

	TuyaEndpoint           string
	TuyaTVMAlias           string
	SberEndpoint           string
	SberTVMAlias           string
	PhilipsEndpoint        string
	XiaomiEndpoint         string
	QuasarEndpoint         string
	QuasarTVMAlias         string
	CloudFunctionsTVMAlias string
}

func (pf *Factory) NewProviderClient(ctx context.Context, origin model.Origin, skillID string) (IProvider, error) {
	if len(skillID) == 0 {
		return nil, fmt.Errorf("cannot get provider, skillId is empty")
	}

	skillInfo, err := pf.SkillInfo(ctx, skillID, origin.User.Ticket)
	if err != nil {
		return nil, err
	}

	switch skillID {
	case model.TUYA:
		return pf.newTuyaProviderClient(ctx, origin, skillInfo)
	case model.SberSkill:
		return pf.newSberProviderClient(ctx, origin, skillInfo)
	case model.QUASAR:
		return pf.newQuasarProviderClient(ctx, origin, skillInfo)
	case model.YANDEXIO:
		return pf.newYandexIOProviderClient(ctx, origin, skillInfo)
	case model.VIRTUAL, model.QUALITY, model.UIQUALITY:
		return nil, nil
	}

	// Yandex Cloud Lambda Hosted provider
	if skillInfo.IsLambda {
		return pf.newJSONRPCProviderClient(ctx, origin, skillInfo)
	}

	// Default oAuth provider
	return pf.newRESTProviderClient(ctx, origin, skillInfo)
}

func (pf *Factory) SkillInfo(ctx context.Context, skillID string, userTicket string) (SkillInfo, error) {
	switch skillID {
	case model.TUYA:
		return SkillInfo{
			HumanReadableName: model.HumanReadableTuyaProviderName,
			SkillID:           model.TUYA,
			Endpoint:          pf.TuyaEndpoint,
			TVMAlias:          pf.TuyaTVMAlias,
			Trusted:           true,
		}, nil
	case model.SberSkill:
		return SkillInfo{
			HumanReadableName: model.HumanReadableSberProviderName,
			SkillID:           model.SberSkill,
			Endpoint:          pf.SberEndpoint,
			TVMAlias:          pf.SberTVMAlias,
			Trusted:           true,
			Public:            true,
		}, nil
	case model.QUASAR:
		quasarHostURL := pf.QuasarEndpoint
		quasarTVMAlias := pf.QuasarTVMAlias
		if contextHostURL, ok := servicehost.GetServiceHostURL(ctx, string(QuasarServiceKey)); ok {
			tvmAlias, knownHost := KnownQuasarProviderHosts[HostURL(contextHostURL)]
			if !knownHost {
				ctxlog.Warnf(ctx, pf.Logger, "Unknown Quasar host: %s", contextHostURL)
				return SkillInfo{}, xerrors.Errorf("unknown Quasar host: %s", contextHostURL)
			}
			quasarHostURL = tools.URLJoin("https://", contextHostURL, "/iot")
			quasarTVMAlias = string(tvmAlias)
			ctxlog.Infof(ctx, pf.Logger, "Using Quasar host: %s = %s", QuasarServiceKey, quasarHostURL)
		}
		return SkillInfo{
			HumanReadableName: model.HumanReadableQuasarProviderName,
			SkillID:           model.QUASAR,
			Endpoint:          quasarHostURL,
			TVMAlias:          quasarTVMAlias,
			Trusted:           true,
			Public:            true,
		}, nil
	case model.YANDEXIO:
		return SkillInfo{
			HumanReadableName: model.HumanReadableYandexIOProviderName,
			SkillID:           model.YANDEXIO,
			Trusted:           true,
			Public:            true,
		}, nil
	case model.VIRTUAL, model.QUALITY, model.UIQUALITY:
		return SkillInfo{SkillID: skillID}, nil
	}

	skillInfo, err := pf.Dialogs.GetSkillInfo(ctx, skillID, userTicket) // ticket is optional in this call
	if err != nil {
		return SkillInfo{}, err
	}

	endpoint := skillInfo.BackendURL
	isLambda := skillInfo.FunctionID != ""
	if isLambda {
		endpoint = fmt.Sprintf("https://functions.yandexcloud.net/%s?integration=raw", skillInfo.FunctionID)
	}
	if skillInfo.SkillID == model.PhilipsSkill && len(pf.PhilipsEndpoint) > 0 {
		endpoint = pf.PhilipsEndpoint
	}
	if skillInfo.SkillID == model.XiaomiSkill && len(pf.XiaomiEndpoint) > 0 {
		endpoint = pf.XiaomiEndpoint
	}

	return SkillInfo{
		ApplicationName:   skillInfo.ApplicationName,
		HumanReadableName: skillInfo.Name,
		Endpoint:          endpoint,
		SkillID:           skillInfo.SkillID,
		IsLambda:          isLambda,
		Trusted:           skillInfo.Trusted,
		Public:            skillInfo.Public,
	}, nil
}

func (pf *Factory) GetSignalsRegistry() *SignalsRegistry {
	return pf.SignalsRegistry
}

func (pf *Factory) newTuyaProviderClient(ctx context.Context, origin model.Origin, skillInfo SkillInfo) (*TuyaProvider, error) {
	tvmServiceTicket, err := pf.Tvm.GetServiceTicketForAlias(ctx, skillInfo.TVMAlias)
	if err != nil {
		return nil, xerrors.Errorf("failed get tvm service ticket for tuya provider client: %w", err)
	}
	authPolicy := NewInternalAuthPolicy(tvmServiceTicket, origin.User.ID)
	signals := pf.SignalsRegistry.GetSignals(ctx, requestsource.GetRequestSource(ctx), skillInfo)
	p := newRESTProvider(origin, skillInfo, pf.Logger, pf.DefaultClient, pf.ZoraClient, authPolicy, signals)
	return &TuyaProvider{RESTProvider: p}, nil
}

func (pf *Factory) newSberProviderClient(ctx context.Context, origin model.Origin, skillInfo SkillInfo) (*SberProvider, error) {
	tvmServiceTicket, err := pf.Tvm.GetServiceTicketForAlias(ctx, skillInfo.TVMAlias)
	if err != nil {
		return nil, xerrors.Errorf("failed get tvm service ticket for sber provider client: %w", err)
	}
	authPolicy := NewInternalAuthPolicy(tvmServiceTicket, origin.User.ID)
	signals := pf.SignalsRegistry.GetSignals(ctx, requestsource.GetRequestSource(ctx), skillInfo)
	p := newRESTProvider(origin, skillInfo, pf.Logger, pf.DefaultClient, pf.ZoraClient, authPolicy, signals)
	return &SberProvider{RESTProvider: p}, nil
}

func (pf *Factory) newQuasarProviderClient(ctx context.Context, origin model.Origin, skillInfo SkillInfo) (IProvider, error) {
	tvmServiceTicket, err := pf.Tvm.GetServiceTicketForAlias(ctx, skillInfo.TVMAlias)
	if err != nil {
		return nil, xerrors.Errorf("failed get TVM service ticket for Quasar Provider client: %w", err)
	}
	authPolicy := NewQuasarAuthPolicy(origin.User.Ticket, tvmServiceTicket, origin.User.ID) // TODO: use InternalPolicy after quasar backend supports X-Ya-User-ID
	signals := pf.SignalsRegistry.GetSignals(ctx, requestsource.GetRequestSource(ctx), skillInfo)
	p := newRESTProvider(origin, skillInfo, pf.Logger, pf.DefaultClient, pf.ZoraClient, authPolicy, signals)
	return newQuasarProvider(p, pf.Bass, pf.Notificator), nil
}

func (pf *Factory) newYandexIOProviderClient(ctx context.Context, origin model.Origin, skillInfo SkillInfo) (IProvider, error) {
	signals := pf.SignalsRegistry.GetSignals(ctx, requestsource.GetRequestSource(ctx), skillInfo)
	return newYandexIOProvider(origin, pf.Logger, skillInfo, signals, pf.Bass, pf.Notificator), nil
}

func (pf *Factory) newRESTProviderClient(ctx context.Context, origin model.Origin, skillInfo SkillInfo) (IProvider, error) {
	if valid := strings.HasPrefix(skillInfo.Endpoint, "http"); !valid {
		return nil, fmt.Errorf("endpoint is empty or has an invalid format: %s", skillInfo.Endpoint)
	}
	socialismSkillInfo := socialism.NewSkillInfo(skillInfo.SkillID, skillInfo.ApplicationName, skillInfo.Trusted)
	token, err := pf.Socialism.GetUserApplicationToken(ctx, origin.User.ID, socialismSkillInfo)
	if err != nil {
		return nil, xerrors.Errorf("failed to get user %d token for app %s from socialism: %w", origin.User.ID, skillInfo.ApplicationName, err)
	}

	var authPolicy authpolicy.HTTPPolicy
	if skillInfo.SkillID == model.XiaomiSkill {
		authPolicy = NewXiaomiAuthPolicy(token, origin.User.ID)
	} else {
		authPolicy = NewOAuthPolicy(token)
	}

	signals := pf.SignalsRegistry.GetSignals(ctx, requestsource.GetRequestSource(ctx), skillInfo)
	p := newRESTProvider(origin, skillInfo, pf.Logger, pf.DefaultClient, pf.ZoraClient, authPolicy, signals)
	return &p, nil
}

func (pf *Factory) newJSONRPCProviderClient(ctx context.Context, origin model.Origin, skillInfo SkillInfo) (IProvider, error) {
	socialismSkillInfo := socialism.NewSkillInfo(skillInfo.SkillID, skillInfo.ApplicationName, skillInfo.Trusted)
	token, err := pf.Socialism.GetUserApplicationToken(ctx, origin.User.ID, socialismSkillInfo)
	if err != nil {
		return nil, xerrors.Errorf("failed to get user %d token for app %s from socialism: %w", origin.User.ID, skillInfo.ApplicationName, err)
	}
	ticket, err := pf.Tvm.GetServiceTicketForAlias(ctx, pf.CloudFunctionsTVMAlias)
	if err != nil {
		return nil, xerrors.Errorf("failed get TVM service ticket for Yandex Cloud request: %w", err)
	}

	httpAuthPolicy := NewCloudFunctionsAuthPolicy(ticket)
	rpcAuthPolicy := NewCloudFunctionsRPCOAuthPolicy(token)
	signals := pf.SignalsRegistry.GetSignals(ctx, requestsource.GetRequestSource(ctx), skillInfo)

	p := newJSONRPCProvider(origin, skillInfo, pf.Logger, pf.DefaultClient, pf.ZoraClient, httpAuthPolicy, rpcAuthPolicy, signals)
	return &p, nil
}
