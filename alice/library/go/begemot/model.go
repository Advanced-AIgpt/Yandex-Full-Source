package libbegemot

import (
	"net/url"
	"strings"

	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type WizardClient string

var (
	MegamindWizardClient WizardClient = "megamind"
	DialogWizardClient   WizardClient = "dialog"
)

type TopLevelDomain string

var (
	RuTLD TopLevelDomain = "ru"
	TrTLD TopLevelDomain = "tr"
	KzTLD TopLevelDomain = "kz"
)

type UserLanguage string

var (
	RuLang UserLanguage = "ru"
	TrLang UserLanguage = "tr"
	KkLang UserLanguage = "kk"
)

type BegemotRule string

var (
	AliceCustomEntities BegemotRule = "AliceCustomEntities"
	AliceNormalizer     BegemotRule = "AliceNormalizer"
	AliceIot            BegemotRule = "AliceIot"
	AliceRequest        BegemotRule = "AliceRequest"
	FstTime             BegemotRule = "FstTime"
	FstDatetime         BegemotRule = "FstDatetime"
	FstDatetimeRange    BegemotRule = "FstDatetimeRange"
)

type BegemotRules []BegemotRule

func (brs BegemotRules) Strings() []string {
	result := make([]string, 0, len(brs))
	for _, br := range brs {
		result = append(result, string(br))
	}
	return result
}

type WizardRequest struct {
	Text           string
	WizardClient   WizardClient
	TopLevelDomain TopLevelDomain
	UserLanguage   UserLanguage
	Rules          BegemotRules
	WizExtra       []string
}

func (r WizardRequest) toQueryParams() url.Values {
	queryParams := url.Values{}
	queryParams.Add("format", "json")
	queryParams.Add("text", r.Text)
	queryParams.Add("tld", string(r.TopLevelDomain))
	queryParams.Add("uil", string(r.UserLanguage))
	queryParams.Add("wizclient", string(r.WizardClient))
	queryParams.Add("rwr", strings.Join(r.Rules.Strings(), ","))
	queryParams.Add("wizextra", strings.Join(r.WizExtra, ","))
	return queryParams
}

type WizardResponse struct {
	OriginalRequest string      `json:"original_request"`
	Rules           WizardRules `json:"rules"`
	// Markup field is also present in response, but it has no clients now
}

type WizardRules struct {
	AliceIOTResult AliceIOTRuleResult `json:"AliceIot"`
	// other rules have no clients now
}

type AliceIOTRuleResult struct {
	Result              *scenarios.TBegemotIotNluResult `json:"Result"`
	NormalizedUtterance string                          `json:"NormalizedUtterance"`
	Err                 string                          `json:"Error"`

	// RawEntities sometimes come as []string and sometimes as string
	// it is better to ignore them because this field has no clients now
	//RawEntities         string                          `json:"RawEntities"`
}

func (r AliceIOTRuleResult) IsEmpty() bool {
	return r.Result == nil
}

func (r AliceIOTRuleResult) Error() error {
	if len(r.Err) > 0 {
		return xerrors.New(r.Err)
	}
	return nil
}
