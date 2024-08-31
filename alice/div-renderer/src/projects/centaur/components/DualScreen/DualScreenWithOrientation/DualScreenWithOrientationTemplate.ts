import { IRequestState } from '../../../../../common/types/common';
import { ContainerBlock, Div, MatchParentSize, SolidBackground, Template } from 'divcard2';
import { createRequestState } from '../../../../../registries/common';
import { ContainerBlockProps } from '../../../helpers/types';

export type IDualScreenWithOrientationTemplateProps = {
    firstDiv: Div[];
    secondDiv: Div[];
    mainColor: string;
    mainColor1: string;
} & Partial<ContainerBlockProps>;

export default function DualScreenWithOrientationTemplate(isInverse = false): [Div, IRequestState] {
    const requestState = createRequestState();

    return [
        new ContainerBlock({
            background: [new SolidBackground({ color: new Template('mainColor') })],
            width: new MatchParentSize(),
            height: new MatchParentSize(),
            items: [
                new ContainerBlock({
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                    items: new Template(isInverse ? 'secondDiv' : 'firstDiv'),
                }),
                new ContainerBlock({
                    background: [new SolidBackground({ color: new Template('mainColor1') })],
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                    items: new Template(isInverse ? 'firstDiv' : 'secondDiv'),
                }),
            ],
        }),
        requestState,
    ];
}
