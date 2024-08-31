package game

type RoundState struct {
	MajorityTypeID              int
	OddTypeID                   int
	OddWord                     string
	MajorityWords               []string
	WantsKnowAnswer             bool
	IsEasy                      bool
	CurrentCountOfMajorityWords int
	CurrentWords                []string
	IsFirstQuestion             bool
}

type State struct {
	GameState    string
	CurrentLevel int
	RoundState   RoundState
	Answers      int
	RightAnswers int
	IsFirstGame  bool
}
