import { google, NAlice } from '../../protos';

export class MMRequest {
    readonly ScenarioResponseBody: NAlice.NScenarios.ITScenarioResponseBody;
    readonly Input: NAlice.NScenarios.ITInput;
    readonly Experiments?: (google.protobuf.IStruct | null);
    readonly RequestId?: (string | null);
    readonly ClientInfo?: (NAlice.ITClientInfoProto | null);
    readonly Capabilities?: (Capabilities | null);

    constructor(
        responseBody: NAlice.NScenarios.ITScenarioResponseBody,
        input: NAlice.NScenarios.ITInput,
        experiments: (google.protobuf.IStruct | null) = null,
        requestId?: (string | null),
        clientInfo: (NAlice.ITClientInfoProto | null) = null,
        capabilities: (Capabilities | null) = null) {
        this.ScenarioResponseBody = responseBody;
        this.Input = input;
        this.RequestId = requestId;
        this.Experiments = experiments;
        this.ClientInfo = clientInfo;
        this.Capabilities = capabilities;
    }
}

export const getAsrTextFromInput = (mmRequest: MMRequest): (string | null) => {
    return mmRequest?.Input?.Voice?.Utterance ?? mmRequest?.Input?.Text?.Utterance ?? null;
};

export class Capabilities {
    // TODO: move from raw capability to object representation
    readonly DivView?: (NAlice.TDivViewCapability | null);

    constructor(DivView?: (NAlice.TDivViewCapability | null)) {
        this.DivView = DivView;
    }
}
