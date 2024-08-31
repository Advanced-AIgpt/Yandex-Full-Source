package bass

type ServiceTextData struct {
	TextActions []string `json:"text_actions"`
}

type ServicePhraseData struct {
	Phrase string `json:"phrase"`
}

type CallbackData struct {
	DID      string `json:"did"`
	UID      string `json:"uid"`
	ClientID string `json:"client_id"`
}
