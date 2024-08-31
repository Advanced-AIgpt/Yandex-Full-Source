import { ContainerBlock, Div, DivBackground, MatchParentSize, SolidBackground } from 'divcard2';
import { colorBlueTextOpacity10 } from '../../../style/constants';
import { MAIN_SCREEN_CORNER_RADIUS } from '../constants';
import getColorSet, { IColorSet } from '../../../style/colorSet';

export type IAbstractCardData = ConstructorParameters<typeof ContainerBlock>[0] & { colorSet?: IColorSet; id: string; };

export const basicCardBackground: DivBackground[] = [
    new SolidBackground({ color: colorBlueTextOpacity10 }),
];

export function BasicCard({ colorSet = getColorSet(), ...props }: IAbstractCardData): Div {
    return new ContainerBlock({
        background: [new SolidBackground({
            color: colorSet.textColorOpacity10,
        })],
        paddings: {
            top: 24,
            right: 24,
            bottom: 24,
            left: 24,
        },
        height: new MatchParentSize({ weight: 1 }),
        width: new MatchParentSize({ weight: 1 }),
        border: {
            corner_radius: MAIN_SCREEN_CORNER_RADIUS,
        },
        ...props,
    });
}
