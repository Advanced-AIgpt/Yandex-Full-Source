package model

func NewRoom(name string) *Room {
	return &Room{
		Name: name,
	}
}

func (r *Room) WithID(id string) *Room {
	r.ID = id
	return r
}
