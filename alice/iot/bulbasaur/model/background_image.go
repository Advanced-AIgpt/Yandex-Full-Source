package model

type BackgroundImageID string

const (
	LivingRoomBackgroundImageID   BackgroundImageID = "living_room"
	BedroomBackgroundImageID      BackgroundImageID = "bedroom"
	KitchenBackgroundImageID      BackgroundImageID = "kitchen"
	ChildrenRoomBackgroundImageID BackgroundImageID = "children_room"
	HallwayBackgroundImageID      BackgroundImageID = "hallway"
	BathroomBackgroundImageID     BackgroundImageID = "bathroom"
	WorkspaceBackgroundImageID    BackgroundImageID = "workspace"
	CountryHouseBackgroundImageID BackgroundImageID = "country_house"
	AllBackgroundImageID          BackgroundImageID = "all"
	BalconyBackgroundImageID      BackgroundImageID = "balcony"
	MyRoomBackgroundImageID       BackgroundImageID = "my_room"
	ScenariosBackgroundImageID    BackgroundImageID = "scenarios"
	FavoriteBackgroundImageID     BackgroundImageID = "favorite"
	Default1BackgroundImageID     BackgroundImageID = "default_1"
	Default2BackgroundImageID     BackgroundImageID = "default_2"
	Default3BackgroundImageID     BackgroundImageID = "default_3"
	Default4BackgroundImageID     BackgroundImageID = "default_4"
)

type BackgroundImage struct {
	ID BackgroundImageID
}
