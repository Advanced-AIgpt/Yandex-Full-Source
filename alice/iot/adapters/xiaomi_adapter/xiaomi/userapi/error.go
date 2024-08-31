package userapi

import "fmt"

type XiaomiError struct {
	Message     string
	Code        int
	Description string
}

func (f XiaomiError) Error() string {
	return fmt.Sprintf("Xiaomi UserApi error: %s: [%d] %s", f.Message, f.Code, f.Description)
}

type Forbidden struct {
	XiaomiError
}

type Unauthorized struct {
	XiaomiError
}
