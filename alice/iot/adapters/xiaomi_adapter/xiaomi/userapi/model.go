package userapi

type UserProfileResult struct {
	Result      string
	Code        int
	Description string
	Data        UserData
}

type UserData struct {
	Nick    string `json:"miliaoNick"`
	UnionID string
}
