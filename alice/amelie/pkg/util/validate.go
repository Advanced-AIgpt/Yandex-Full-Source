package util

import "fmt"

func ValidateNotNil(items ...interface{}) error {
	for _, item := range items {
		if item == nil {
			return fmt.Errorf("item %v is nil", item)
		}
	}
	return nil
}
