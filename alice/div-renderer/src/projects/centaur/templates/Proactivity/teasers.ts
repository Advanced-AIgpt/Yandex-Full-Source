import { NAlice } from '../../../../protos';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import PartSimpleText from '../../components/DualScreen/partComponents/PartSimpleText';
import getColorSet from '../../style/colorSet';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { TopLevelCard } from '../../helpers/helpers';
import EmptyDiv from '../../components/EmptyDiv';
import { colorMoreThenBlackOpacity48 } from '../../style/constants';

export default function ProactivityTeaser(data: NAlice.NData.ITProactivityTeaserData, mmRequest: MMRequest, requestState: IRequestState) {
    return TopLevelCard({
        log_id: 'proactivity_scenario',
        states: [
            {
                state_id: 0,
                div: proactivityTeaserDiv(data, mmRequest, requestState),
            },
        ],
    }, requestState);
}

function proactivityTeaserDiv(data: NAlice.NData.ITProactivityTeaserData, _: MMRequest, requestState: IRequestState) {
    const description = data.Description ?? '';
    const text = data.Title ?? undefined;
    const image = data.BackgroundImageUrl ?? '';
    const theme = getColorSet({
        id: text,
    });

    return DualScreen({
        requestState,
        firstDiv: [new EmptyDiv()],
        secondDiv: PartSimpleText({
            colorSet: theme,
            text,
            subText: description,
            backgroundColor: colorMoreThenBlackOpacity48,
            backgroundImage: image,
        }),
        mainColor1: theme.mainColor1,
        mainColor: theme.mainColor,
    });
}
