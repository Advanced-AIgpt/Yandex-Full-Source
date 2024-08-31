import { ContainerBlock } from 'divcard2';
import { NAlice } from '../../../../protos';
import { TopLevelCard } from '../../../centaur/helpers/helpers';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import Greetings from '../../components/Greetings/Greetings';
type ExampleData = NAlice.NData.ITExampleScenarioData;

export default function ExampleDiv({ hello }: ExampleData, _: MMRequest, requestState: IRequestState) {
    return TopLevelCard({
        log_id: 'example_card',
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    items: [
                        Greetings({
                            hello: hello ?? ' ',
                        }, requestState),
                    ],
                }),
            },
        ],
    }, requestState);
}
