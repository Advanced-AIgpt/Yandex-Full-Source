package libquasar

import (
	"bytes"
	"context"
	"encoding/base64"
	"encoding/json"
	"io"
	"sort"
	"strings"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type authPolicyFactory struct {
	TVMDstID tvm.ClientID
	TVM      tvm.Client
}

func (f authPolicyFactory) NewAuthPolicy(ctx context.Context, userTicket string) (authpolicy.TVMPolicy, error) {
	serviceTicket, err := f.TVM.GetServiceTicketForID(ctx, f.TVMDstID)
	if err != nil {
		return authpolicy.TVMPolicy{}, xerrors.Errorf("failed to get service tvm ticket: %v", err)
	}

	tvmPolicy := authpolicy.TVMPolicy{
		UserTicket:    userTicket,
		ServiceTicket: serviceTicket,
	}
	return tvmPolicy, nil
}

type DeviceKey struct {
	DeviceID string `json:"device_id"`
	Platform string `json:"platform"`
}

type SetDevicesConfigPayload struct {
	Devices []SetDeviceConfig `json:"devices"`
}

type DeviceConfigResult struct {
	Status  string `json:"status"`
	Version string `json:"version"`
	Config  Config `json:"config"`
}

type SetDeviceConfig struct {
	DeviceKey
	FromVersion string `json:"from_version"`
	Config      Config `json:"config"`
}

type SetDeviceConfigResult struct {
	Status  string         `json:"status"`
	Devices DeviceVersions `json:"devices"`
}

type DeviceVersion struct {
	DeviceID string `json:"device_id"`
	Version  string `json:"version"`
}

type DeviceVersions []DeviceVersion

func (versions DeviceVersions) Clone() DeviceVersions {
	res := make(DeviceVersions, len(versions))
	copy(res, versions)
	return res
}

func (versions *DeviceVersions) FromJoined(s string) error {
	dec := base64.NewDecoder(base64.RawURLEncoding, strings.NewReader(s))
	b, _ := io.ReadAll(dec)
	err := json.Unmarshal(b, versions)
	return err
}

func (versions DeviceVersions) GetByDeviceID(id string) (version string, ok bool) {
	for _, dev := range versions {
		if dev.DeviceID == id {
			return dev.Version, true
		}
	}
	return "", false
}

func (versions DeviceVersions) JoinVersions() string {
	// sort need for repeatable version string independent of
	// device order in slice
	clone := versions.Clone()
	sort.Sort(clone)

	b, _ := json.Marshal(clone)
	buf := &bytes.Buffer{}
	enc := base64.NewEncoder(base64.RawURLEncoding, buf)
	_, _ = io.Copy(enc, bytes.NewReader(b))
	_ = enc.Close()
	return buf.String()
}

func (versions DeviceVersions) Len() int {
	return len(versions)
}

func (versions DeviceVersions) Less(i, j int) bool {
	return versions[i].DeviceID < versions[j].DeviceID
}

func (versions DeviceVersions) Swap(i, j int) {
	versions[i], versions[j] = versions[j], versions[i]
}

type IotDeviceInfoResponse struct {
	Status  string          `json:"status"`
	Devices []IotDeviceInfo `json:"devices"`
}

type IotDeviceInfo struct {
	ID       string                       `json:"id"`
	Platform string                       `json:"platform"`
	Config   IotDeviceInfoVersionedConfig `json:"config"`
	Group    *GroupInfo                   `json:"group,omitempty"`
}

func (info IotDeviceInfo) DeviceKey() DeviceKey {
	return DeviceKey{
		DeviceID: info.ID,
		Platform: info.Platform,
	}
}

type IotDeviceInfoVersionedConfig struct {
	Content Config `json:"content"`
	Version string `json:"version"`
}

type GroupInfo struct {
	ID      uint64       `json:"id"`
	Name    string       `json:"name"`
	Secret  string       `json:"secret"`
	Config  Config       `json:"config"`
	Devices GroupDevices `json:"devices"`
}

type GroupDevice struct {
	ID       string          `json:"id"`
	Platform string          `json:"platform"`
	Role     GroupDeviceRole `json:"role"`
}

type GroupDevices []GroupDevice

func (groupDevices GroupDevices) DeviceByID(id string) (GroupDevice, bool) {
	for _, groupDevice := range groupDevices {
		if groupDevice.ID == id {
			return groupDevice, true
		}
	}
	return GroupDevice{}, false
}

type GroupCreateRequest struct {
	Name    string       `json:"name"`
	Secret  string       `json:"secret"`
	Config  Config       `json:"config"`
	Devices GroupDevices `json:"devices"`
}

type GroupCreateResponse struct {
	Status  string `json:"status"`
	GroupID uint64 `json:"group_id"`
}

type GroupUpdateRequest struct {
	ID uint64 `json:"id"`
	GroupCreateRequest
}

type GroupDeviceRole string

const (
	LeaderGroupDeviceRole   GroupDeviceRole = "leader"
	FollowerGroupDeviceRole GroupDeviceRole = "follower"
)

type EncryptPayloadRequest struct {
	Payload    string
	DeviceID   string
	Platform   string
	RSAPadding RSAPadding
}

type EncryptPayloadResponse struct {
	Base64CipherText string `json:"ciphertext"` // encrypted_with_AES_128_CBC(data, random_key_P),
	Base64Signature  string `json:"M"`          // signed_with_HMAC_SHA256(ciphertext, random_key_P)
	Base64SessionKey string `json:"C"`          // encrypted_with_RSA(random_key_P)
}

type RSAPadding int

const (
	Pkcs1RSAPadding     RSAPadding = 1
	Pkcs1OaepRSAPadding RSAPadding = 2
)
