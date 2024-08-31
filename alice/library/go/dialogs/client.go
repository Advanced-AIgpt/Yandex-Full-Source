package dialogs

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/karlseguin/ccache/v2"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/retryablehttp"
	"a.yandex-team.ru/alice/library/go/servicehost"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	xYaUserTicket    = "X-Ya-User-Ticket"
	xYaServiceTicket = "X-Ya-Service-Ticket"
)

type Client struct {
	hostInfo    HostInfo
	client      *retryablehttp.Client
	tvm         tvm.Client
	Logger      log.Logger
	signals     signals
	cache       *ccache.Cache
	cachePolicy CachePolicy
}

func (d *Client) Init(tvmClient tvm.Client, tvmAlias string, dialogsURL string, registry metrics.Registry, policy CachePolicy) {
	d.Logger.Info("Initializing Dialogs Client")

	if tvmAlias != "" {
		d.hostInfo.TVMAlias = HostTVMAlias(tvmAlias)
	} else {
		d.Logger.Infof("Dialogs TVMAlias is not specified, use production alias by default")
		d.hostInfo.TVMAlias = ProductionTVMAlias
	}
	if dialogsURL != "" {
		d.hostInfo.URL = HostURL(dialogsURL)
	} else {
		d.Logger.Infof("Dialogs URL is not specified, use production URL by default")
		d.hostInfo.URL = HostURL(tools.URLJoin("https://", string(ProductionURL), "/api"))
	}

	d.cache = ccache.New(ccache.Configure())
	d.cachePolicy = policy

	d.tvm = tvmClient

	d.client = retryablehttp.NewClient()
	d.client.Logger = d.Logger
	d.client.HTTPClient.Timeout = time.Second * 3
	d.client.RetryMax = 5
	d.client.RetryWaitMin = 0 * time.Millisecond
	d.client.RetryWaitMax = 10 * time.Millisecond

	if registry != nil {
		d.signals = newSignals(registry)
	} else {
		d.Logger.Warn("Dialogs Client received nil registry, metrics will not be recorded.")
	}
	d.Logger.Info("Dialogs Client was successfully initialized.")
}

func (d *Client) SetResponseLogHook(hook func(log.Logger, *http.Response)) {
	d.client.ResponseLogHook = hook
}

func (d *Client) getHostInfo(ctx context.Context) HostInfo {
	hostInfo := d.hostInfo
	if contextHostURL, ok := servicehost.GetServiceHostURL(ctx, string(DialogsServiceKey)); ok {
		if tvmAlias, exist := KnownDialogsHosts[HostURL(contextHostURL)]; exist {
			hostInfo.URL = HostURL(tools.URLJoin("https://", contextHostURL, "/api"))
			hostInfo.TVMAlias = tvmAlias
			ctxlog.Infof(ctx, d.Logger, "Using Dialogs host: %s = %s", DialogsServiceKey, hostInfo.URL)
		}
	}
	return hostInfo
}

func (d *Client) AuthorizeSkillOwner(ctx context.Context, userID uint64, skillID, userTicket string) (AuthorizationData, error) {
	authData := AuthorizationData{
		UserID:  userID,
		SkillID: skillID,
		Success: false,
	}
	skillInfo, err := d.getSkillInfo(ctx, skillID, userTicket, NoCachePolicy)
	if err != nil {
		return authData, xerrors.Errorf("authorization failed, unable to get skill info: %w", err)
	}
	authData.Success = skillInfo.AccessToSkillTesting.HasAccess
	return authData, nil
}

func (d *Client) GetSkillCertifiedDevices(ctx context.Context, skillID string, ticket string) (CertifiedDevices, error) {
	resp, err := d.simpleHTTPRequest(ctx, ticket, http.MethodGet, tools.URLJoin("/external/v2/skills/", skillID, "/certified-devices"), nil, d.signals.getSkillCertifiedDevices)
	if err != nil {
		return CertifiedDevices{Categories: []CertifiedCategory{}}, xerrors.Errorf("request to Dialogs failed: %w", err)
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return CertifiedDevices{Categories: []CertifiedCategory{}}, xerrors.Errorf("request to Dialogs failed: [%d] %s: %w", resp.StatusCode, resp.Status, err)
	}
	ctxlog.Infof(ctx, d.Logger, "got raw response from dialogs, status code: %d, status: %s, body: %s", resp.StatusCode, resp.Status, body)

	var response certifiedDevicesResponse
	err = json.Unmarshal(body, &response)
	if err != nil {
		return CertifiedDevices{Categories: []CertifiedCategory{}}, xerrors.Errorf("can't unmarshal response from Dialogs: [%d] %s: %w", resp.StatusCode, resp.Status, err)
	}
	switch resp.StatusCode {
	case 200:
		break
	case 400, 404:
		if response.Message != "" {
			var errCause error
			switch response.Message {
			case "skill not found":
				errCause = &SkillNotFoundError{}
			default:
				errCause = fmt.Errorf(response.Message)
			}
			return CertifiedDevices{Categories: []CertifiedCategory{}}, xerrors.Errorf("failed to get skill certified devices from Dialogs: %w", errCause)
		}
		return CertifiedDevices{Categories: []CertifiedCategory{}}, xerrors.Errorf("request to Dialogs failed: [%d] %s: %w", resp.StatusCode, resp.Status, err)
	default:
		return CertifiedDevices{Categories: []CertifiedCategory{}}, fmt.Errorf("request to Dialogs failed: [%d] %s", resp.StatusCode, resp.Status)
	}
	return response.CertifiedDevices, nil
}

func (d *Client) getSkillInfo(ctx context.Context, skillID string, ticket string, cachePolicy CachePolicy) (*dialogResponse, error) {
	if cachePolicy.UseCache(skillID) {
		item := d.cache.Get(skillID)
		if item != nil && !item.Expired() {
			value := item.Value().(dialogResponse)
			ctxlog.Debug(ctx, d.Logger, "cached_response", log.Any("skill_id", skillID))
			return &value, nil
		}
	}

	resp, err := d.simpleHTTPRequest(ctx, ticket, http.MethodGet, tools.URLJoin("/external/v2/skills/", skillID), nil, d.signals.getSkill)
	if err != nil {
		return nil, xerrors.Errorf("request to Dialogs failed: %w", err)
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("request to Dialogs failed: [%d] %s: %w", resp.StatusCode, resp.Status, err)
	}
	ctxlog.Infof(ctx, d.Logger, "got raw response from dialogs, status code: %d, status: %s, body: %s", resp.StatusCode, resp.Status, body)

	var skill dialogResponse
	err = json.Unmarshal(body, &skill)
	if err != nil {
		return nil, xerrors.Errorf("can't unmarshal response from Dialogs: [%d] %s: %w", resp.StatusCode, resp.Status, err)
	}
	switch resp.StatusCode {
	case 200:
		break
	case 400, 404:
		if skill.Error.Message != "" {
			var errCause error
			switch skill.Error.Message {
			case "skill not found":
				errCause = &SkillNotFoundError{}
			default:
				errCause = fmt.Errorf(skill.Error.Message)
			}
			return nil, xerrors.Errorf("failed to get skill info from Dialogs: %w", errCause)
		}
		return nil, xerrors.Errorf("request to Dialogs failed: [%d] %s: %w", resp.StatusCode, resp.Status, err)
	default:
		return nil, fmt.Errorf("request to Dialogs failed: [%d] %s", resp.StatusCode, resp.Status)
	}

	if cachePolicy.SaveToCache(skill, skillID) {
		d.cache.Set(skillID, skill, 1*time.Hour)
	}

	return &skill, nil
}

func (d *Client) GetSkillInfo(ctx context.Context, skillID string, ticket string) (_ *SkillInfo, err error) {
	skill, err := d.getSkillInfo(ctx, skillID, ticket, d.cachePolicy)
	if err != nil {
		return nil, err
	}
	userID, err := strconv.ParseUint(skill.UserID, 10, 64)
	if err != nil {
		d.Logger.Warnf("Can't parse userId from dialogs response, expected valid id with type uint64, got %d", userID)
	}

	return &SkillInfo{
		UserID:          userID,
		SkillID:         skillID,
		Channel:         skill.Channel,
		ApplicationName: skill.AccountLinking.ApplicationName,
		ClientID:        skill.AccountLinking.ClientID,
		BackendURL:      skill.BackendURL,
		LogoAvatarID:    skill.Logo.AvatarID,
		Description:     skill.Description,
		Name:            skill.Name,
		DeveloperName:   skill.DeveloperName,
		SecondaryTitle:  skill.SecondaryTitle,
		Trusted:         skill.Trusted,
		Public:          skill.SkillAccess == "public",
		FunctionID:      skill.FunctionID,
		RatingHistogram: skill.RatingHistogram,
		AverageRating:   skill.AverageRating,
		UserReview:      skill.UserReview,
	}, nil
}

func (d *Client) GetSmartHomeSkills(ctx context.Context, ticket string) ([]SkillShortInfo, error) {
	resp, err := d.simpleHTTPRequest(ctx, ticket, http.MethodGet, "/catalogue/v1/smart_home/get_native_skills", nil, d.signals.getSmartHomeSkills)
	if err != nil {
		return nil, xerrors.Errorf("request to Dialogs failed: %w", err)
	}
	defer func() { _ = resp.Body.Close() }()

	response := struct {
		Result []Skill
		Error  struct {
			Message string
		}
	}{}

	switch resp.StatusCode {
	case 200:
		break
	case 400, 404:
		body, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return nil, xerrors.Errorf("request to Dialogs failed: [%d] %s: %w", resp.StatusCode, resp.Status, err)
		}
		err = json.Unmarshal(body, &response)
		if err != nil {
			return nil, xerrors.Errorf("request to Dialogs failed: [%d] %s: %w", resp.StatusCode, resp.Status, err)
		}
		if response.Error.Message != "" {
			return nil, fmt.Errorf("failed to get skills from Dialogs: %s", response.Error.Message)
		}
		return nil, xerrors.Errorf("request to Dialogs failed: [%d] %s: %w", resp.StatusCode, resp.Status, err)
	default:
		return nil, fmt.Errorf("request to Dialogs failed: [%d] %s", resp.StatusCode, resp.Status)
	}

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read response body from Dialogs: %w", err)
	}

	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse json Dialogs response: %w", err)
	}

	skills := make([]SkillShortInfo, 0, len(response.Result))
	for _, skill := range response.Result {
		skills = append(skills, skill.toSkillShortInfo())
	}

	return skills, nil
}

func (d *Client) simpleHTTPRequest(ctx context.Context, tvmUserTicket, httpMethod, route string, payload []byte, signals quasarmetrics.RouteSignalsWithTotal) (*http.Response, error) {
	hostInfo := d.getHostInfo(ctx) // for servicehost
	url := tools.URLJoin(string(hostInfo.URL), route)
	req, err := retryablehttp.NewRequest(httpMethod, url, payload)
	if err != nil {
		return nil, xerrors.Errorf("failed to create request to %s: %w", url, err)
	}
	// Add headers
	tvmServiceTicket, err := d.tvm.GetServiceTicketForAlias(ctx, string(hostInfo.TVMAlias))
	if err != nil {
		return nil, xerrors.Errorf("failed get TVM service ticket for Dialogs request: %w", err)
	}
	req.Header.Add(xYaServiceTicket, tvmServiceTicket)
	if len(tvmUserTicket) > 0 {
		// dialogs request fails with empty header - without header it only loses info about skill ownership
		req.Header.Add(xYaUserTicket, tvmUserTicket)
	}

	return d.client.DoWithHTTPMetrics(req.WithContext(ctx), signals)
}

func isDraft(skillID string) bool {
	return strings.HasSuffix(skillID, "-draft")
}
