package philips

// If bridge is offline then philips cloud timeouts...
type BridgeOfflineError struct{}

func (e *BridgeOfflineError) Error() string {
	return "BRIDGE_OFFLINE"
}

type UnauthorizedError struct{}

func (e *UnauthorizedError) Error() string {
	return "UNAUTHORIZED"
}
