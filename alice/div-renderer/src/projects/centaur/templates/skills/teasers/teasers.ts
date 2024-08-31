import { NAlice } from '../../../../../protos';
import { MMRequest } from '../../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../../common/types/common';
import { TopLevelCard } from '../../../helpers/helpers';
import getColorSet from '../../../style/colorSet';
import { ExpFlags } from '../../../expFlags';
import { DualScreen } from '../../../components/DualScreen/DualScreen';
import EmptyDiv from '../../../components/EmptyDiv';
import PartSimpleText from '../../../components/DualScreen/partComponents/PartSimpleText';
import { colorMoreThenBlackOpacity48 } from '../../../style/constants';
import PartImage from '../../../components/DualScreen/partComponents/PartImage';

export default function SkillTeaser(data: NAlice.NData.ITDialogovoSkillTeaserData, mmRequest:MMRequest, requestState: IRequestState) {
    return TopLevelCard({
        log_id: 'skill_scenario',
        states: [
            {
                state_id: 0,
                div: skillTeaserDiv(data, mmRequest, requestState),
            },
        ],
    }, requestState);
}

function skillTeaserDiv(data: NAlice.NData.ITDialogovoSkillTeaserData, _:MMRequest, requestState: IRequestState) {
    const teaserSource = {
        logo: data.SkillInfo?.Logo ?? '',
        name: data.SkillInfo?.Name ?? 'Навык',
    };
    const text = data.Title ?? undefined;
    const subText = data.Text ?? undefined;
    const image = data.ImageUrl ?? '';
    const theme = getColorSet({
        id: text,
    });

    if (requestState.hasExperiment(ExpFlags.extendedNewsDesignWithDoubleScreen2)) {
        return DualScreen({
            requestState,
            firstDiv: [new EmptyDiv()],
            secondDiv: PartSimpleText({
                colorSet: theme,
                text,
                subText,
                source: teaserSource,
                backgroundColor: colorMoreThenBlackOpacity48,
                backgroundImage: image,
            }),
            mainColor1: theme.mainColor1,
            mainColor: theme.mainColor,
        });
    }

    return DualScreen({
        requestState,
        firstDiv: PartSimpleText({
            colorSet: theme,
            text,
            subText,
            source: teaserSource,
        }),
        secondDiv: PartImage({
            imageUrl: image,
        }),
        mainColor1: theme.mainColor1,
        mainColor: theme.mainColor,
    });
}
