import { RequestError } from '@yandex-int/apphost-lib';
import { logger } from '../../../common/logger';
import { IRequestState } from '../../../common/types/common';
import { OneOfKey, templates } from '../../../projects';
import { ExpFlags } from '../../../projects/centaur/expFlags';
import ScenarioDataPrettyJSON from '../../../projects/example/templates/ScenarioDataPrettyJSON/ScenarioDataPrettyJSON';
import { NAlice } from '../../../protos';

interface TryGetDivTemplateFnParams {
    requestState: IRequestState;
    reqLog: typeof logger;
    data: NAlice.NRenderer.TDivRenderData;
    templateId: OneOfKey;
}
export const tryGetDivTemplateFn = ({ reqLog, requestState, data, templateId }: TryGetDivTemplateFnParams) => {
    const templateFn = (() => {
        if (requestState.hasExperiment(`${ExpFlags.scenarioDataPrettyJSON}=${templateId}` as ExpFlags.scenarioDataPrettyJSON)) {
            return ScenarioDataPrettyJSON;
        }

        return templates[templateId];
    })();

    // если нет шаблона - это нормально, но только под флагом :)
    if (!templateFn && requestState.hasExperiment(ExpFlags.templateMayNoExist) === false) {
        reqLog.warn('Invalid proto template id', {
            payload: data,
        });

        throw new RequestError(`templateId ${templateId} not found`);
    }

    return templateFn;
};
