import { exampleTemplateHelper } from '../../index';
import { IRequestState } from '../../../../common/types/common';

export default function Greetings(
    props: Parameters<typeof exampleTemplateHelper.greetings>[0],
    requestState: IRequestState,
) {
    requestState.localTemplates.add('greetings');
    return exampleTemplateHelper.greetings(props);
}
