import { DualScreen } from '../../components/DualScreen/DualScreen';
import { IRequestState } from '../../../../common/types/common';
import PartBasicTopCenterBottom from '../../components/DualScreen/partComponents/PartBasicTopCenterBottom';
import { TextBlock } from 'divcard2';
import { text28m } from '../../style/Text/Text';
import EmptyDiv from '../../components/EmptyDiv';
import getColorSet from '../../style/colorSet';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';

export default function RouteResponseEmpty(requestState: IRequestState) {
    const colorSet = getColorSet();

    return CloseButtonWrapper({
        div: DualScreen({
            requestState,
            mainColor: colorSet.mainColor,
            mainColor1: colorSet.mainColor1,
            firstDiv: PartBasicTopCenterBottom({
                middleDivItems: [
                    new TextBlock({
                        ...text28m,
                        text: 'Не удалось построить маршрут',
                    }),
                ],
            }),
            secondDiv: [new EmptyDiv()],
        }),
    });
}
