package ships

type MyCell struct {
	ShipIndex int
	Used      bool
	Status    int
}

type UserCell int

const (
	DefaultCell  = 0
	AwayShipCell = 1
	NearShipCell = 2
	InjuredCell  = 3
	KilledCell   = 4
)

type Cell struct {
	X int
	Y int
}

type Ship struct {
	Status    int
	Cells     []Cell
	LifeCells int
}

var Letters = []string{"А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "К"}

func GetLetters() []string {
	return Letters
}
