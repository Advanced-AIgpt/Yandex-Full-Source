package admin

type Request struct {
	SkillIds []string `json:"params"`
}

func ExtractSkillIds(requests []Request) []string {
	skillIds := make([]string, len(requests))
	for i, request := range requests {
		// BASS is sending us array of arrays [][]skillIds
		// In these arrays only first element (skillId) is present.
		// we iterate over all arrays
		// and pick the first element (skillId), ignoring other information
		skillIds[i] = request.SkillIds[0]
	}
	return skillIds
}
