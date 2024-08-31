import { upperFirst } from 'lodash';
import { NAlice } from '../../../../protos';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { TopLevelCard } from '../../helpers/helpers';
import ConversationDiv from './ConversationDiv';
import { boltalkaColorSet } from '../../style/colorSet';

function dataAdapter(_: NAlice.NData.ITConversationData, mmRequest: MMRequest) {
    return {
        requestText: upperFirst((mmRequest.Input.Voice?.AsrData && mmRequest.Input.Voice?.AsrData[0].Utterance) ||
            mmRequest.Input.Text?.RawUtterance ||
            mmRequest.Input.Text?.Utterance ||
            ' '),
        responseText: (mmRequest.ScenarioResponseBody.Layout?.Cards && mmRequest.ScenarioResponseBody.Layout?.Cards[0].Text) || ' ',
        colorSet: boltalkaColorSet,
        response: mmRequest.ScenarioResponseBody,
    };
}

export default function Conversation(
    data: NAlice.NData.ITConversationData,
    mmRequest: MMRequest,
    requestState: IRequestState,
) {
    return TopLevelCard({
        log_id: 'conversation-page',
        states: [
            {
                state_id: 0,
                div: ConversationDiv(dataAdapter(data, mmRequest), requestState),
            },
        ],
    }, requestState);
}
