import { IncomingProtobufJSChunk } from '@yandex-int/apphost-lib';
import { MMRequest } from '../../common/helpers/MMRequest';
import { NAlice } from '../../protos';
import { trackFn } from './trackPerformance';
import { putTimeToGetMMRequest } from '../../solomon';
import { logger } from '../../common/logger';

export const createMMRequest = trackFn((chunks: IncomingProtobufJSChunk): MMRequest => {
    const scenarioResponseBody = chunks.getFirstItem('mm_scenario_response')?.parseProto(NAlice.NScenarios.TScenarioRunResponse)?.ResponseBody ??
        chunks.getFirstItem('mm_scenario_run_response')?.parseProto(NAlice.NScenarios.TScenarioRunResponse)?.ResponseBody ??
        chunks.getFirstItem('mm_scenario_continue_response')?.parseProto(NAlice.NScenarios.TScenarioContinueResponse)?.ResponseBody ??
        chunks.getFirstItem('mm_scenario_apply_response')?.parseProto(NAlice.NScenarios.TScenarioApplyResponse)?.ResponseBody ?? {};

    const runRequest = chunks.getFirstItem('mm_scenario_request')?.parseProto(NAlice.NScenarios.TScenarioRunRequest) ??
        chunks.getFirstItem('mm_scenario_run_request')?.parseProto(NAlice.NScenarios.TScenarioRunRequest);
    const scenarioApplyRequest = chunks.getFirstItem('mm_scenario_apply_request')?.parseProto(NAlice.NScenarios.TScenarioApplyRequest);
    const scenarioCombinatorRequest = chunks.getFirstItem('mm_combinator_request')?.parseProto(NAlice.NScenarios.TCombinatorRequest);

    const input = runRequest?.Input ??
        scenarioApplyRequest?.Input ??
        scenarioCombinatorRequest?.Input ?? {};

    const experiments = runRequest?.BaseRequest?.Experiments ??
        scenarioApplyRequest?.BaseRequest?.Experiments ??
        scenarioCombinatorRequest?.BaseRequest?.Experiments ?? {};

    const clientInfo = runRequest?.BaseRequest?.ClientInfo ??
        scenarioApplyRequest?.BaseRequest?.ClientInfo ??
        scenarioCombinatorRequest?.BaseRequest?.ClientInfo ?? {};

    // Parse capabilities
    const envStateDataSource = chunks.getFirstItem('datasource_ENVIRONMENT_STATE')?.parseProto(NAlice.NScenarios.TDataSource)?.EnvironmentState;
    const masterDeviceCapabilities = envStateDataSource?.Endpoints
        ?.find(endpoint => endpoint.Id === clientInfo?.DeviceId)?.Capabilities;

    const divViewCapabilityAny = masterDeviceCapabilities?.find(capability => capability.type_url == 'type.googleapis.com/NAlice.TDivViewCapability');

    // DEMO FOR CENTAUR-1315
    // Remove me
    if (divViewCapabilityAny?.value) {
        logger.debug(`List of known device templates: ${NAlice.TDivViewCapability.decode(divViewCapabilityAny.value)?.State?.GlobalTemplatesCache}`);
    }

    const result = new MMRequest(
        scenarioResponseBody,
        input,
        experiments,
        runRequest?.BaseRequest?.RequestId,
        clientInfo,
        {
            DivView: divViewCapabilityAny?.value && NAlice.TDivViewCapability.decode(divViewCapabilityAny.value),
        },
    );

    return result;
}, ({ duration }) => putTimeToGetMMRequest(duration));
