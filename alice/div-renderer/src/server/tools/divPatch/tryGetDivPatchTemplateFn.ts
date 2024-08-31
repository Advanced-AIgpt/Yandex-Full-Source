import { RequestError } from '@yandex-int/apphost-lib';
import { logger } from '../../../common/logger';
import { OneOfKey } from '../../../projects';
import { templatesDivPatch } from '../../../projects/centaur/templates';
import { NAlice } from '../../../protos';

interface TryGetDivPatchTemplateFnParams {
    reqLog: typeof logger;
    data: NAlice.NRenderer.TDivRenderData;
    templateId: OneOfKey;
}
export const tryGetDivPatchTemplateFn = ({ reqLog, data, templateId }: TryGetDivPatchTemplateFnParams) => {
    const templateFn = templatesDivPatch[templateId];

    if (!templateFn) {
        reqLog.warn('Invalid proto div patch template id', {
            payload: data,
        });

        throw new RequestError(`div patch templateId ${templateId} not found`);
    }

    return templateFn;
};
