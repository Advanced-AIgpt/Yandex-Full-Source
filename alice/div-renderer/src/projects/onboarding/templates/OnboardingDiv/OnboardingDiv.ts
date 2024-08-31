import { GalleryBlock } from 'divcard2';
import { NAlice } from '../../../../protos';
import { TopLevelCard } from '../../../centaur/helpers/helpers';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import Greetings from '../../components/Greetings/Greetings';
import { greetingsPalette } from '../../components/Greetings/GreetingsColors';

type GreetingsCardData = NAlice.NData.ITGreetingsCardData;

export default function(data: GreetingsCardData, _: MMRequest, requestState: IRequestState) {
    return TopLevelCard({
        log_id: 'onboarding_card',
        states: [
            {
                state_id: 0,
                div: new GalleryBlock({
                    column_count: 2,
                    item_spacing: 0,
                    items: Greetings({
                        data,
                    }, requestState),
                    paddings: {
                        left: 24,
                        right: 24,
                    },
                    margins: {
                        bottom: 10,
                    },
                }),
            },
        ],
    }, requestState, greetingsPalette.palette);
}
