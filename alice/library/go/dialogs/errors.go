package dialogs

type ErrorCode string

const (
	SkillNotFound ErrorCode = "SKILL_NOT_FOUND"
)

func EC(ec ErrorCode) *ErrorCode { return &ec }

type SkillNotFoundError struct{}

func (s *SkillNotFoundError) Error() string {
	return string(SkillNotFound)
}
