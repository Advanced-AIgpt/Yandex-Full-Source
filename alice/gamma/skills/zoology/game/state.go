package game

type State struct {
	GameState  string
	RoundState RoundState
	IsPromo    bool
}

type RoundState struct {
	Questions       int
	RightAnswers    int
	CurrentType     string
	PreviousAnimal  int
	LastAnswerType  string
	CurrentQuestion *Question
}
