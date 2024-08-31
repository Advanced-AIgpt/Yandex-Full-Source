package sup

import (
	"fmt"
)

type UUIDReceiver struct {
	UUID string
}

func (r UUIDReceiver) isSupReceiver() {}

func (r UUIDReceiver) MarshalJSON() ([]byte, error) {
	return []byte(fmt.Sprintf(`"uuid: %s"`, r.UUID)), nil
}

type DIDReceiver struct {
	DID string
}

func (r DIDReceiver) isSupReceiver() {}

func (r DIDReceiver) MarshalJSON() ([]byte, error) {
	return []byte(fmt.Sprintf(`"did: %s"`, r.DID)), nil
}

type UIDReceiver struct {
	UID string
}

func (r UIDReceiver) isSupReceiver() {}

func (r UIDReceiver) MarshalJSON() ([]byte, error) {
	return []byte(fmt.Sprintf(`"uid: %s"`, r.UID)), nil
}

type TagReceiver struct {
	Tag string
}

func (r TagReceiver) isSupReceiver() {}

func (r TagReceiver) MarshalJSON() ([]byte, error) {
	return []byte(fmt.Sprintf(`"tag: %s"`, r.Tag)), nil
}

type YtTableReceiver struct {
	YtTablePath string
}

func (r YtTableReceiver) isSupReceiver() {}

func (r YtTableReceiver) MarshalJSON() ([]byte, error) {
	return []byte(fmt.Sprintf(`"yt: %s"`, r.YtTablePath)), nil
}
