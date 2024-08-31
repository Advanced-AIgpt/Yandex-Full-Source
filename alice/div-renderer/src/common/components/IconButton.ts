import {
    ContainerBlock,
    DivBackground,
    DivStateBlock,
    FixedSize,
    IDivAction, IDivAnimation,
    IDivStateBlockState,
    ImageBlock, WrapContentSize,
} from 'divcard2';

interface IIconButtonState {
    stateId: string;
    iconUrl?: string;
    background?: DivBackground[];
    actions: IDivAction[];
    alpha?: number;
    animationIn?: IDivAnimation;
    animationOut?: IDivAnimation;
    color?: string;
}

interface IIconButtonProps {
    size: number;
    iconSize: number;
    iconUrl: string;
    background?: DivBackground[];
    actions?: IDivAction[];
    states?: IIconButtonState[];
    id: string;
    alpha?: number;
    margins?: {
        top?: number,
        left?: number;
        right?: number;
        bottom?: number;
    };
    animationSpaceSize?: number;
    actionAnimation?: IDivAnimation;
    defaultStateId?: string;
    color?: string;
    iconType?: 'circle' | 'square';
    alignment_vertical?: 'top' | 'center' | 'bottom';
    alignment_horizontal?: 'left' | 'center' | 'right';
}

/**
 * Компонент для вставки одинаковых иконок с/без круглым фоном.
 * Поддерживается возможность простого создания состояний
 * @param size Размер всей кнопки
 * @param iconSize Размер иконки внутри кнопки
 * @param iconUrl Ссылка на иконку кнопки
 * @param background Фон
 * @param states Состояния, с их помощью можно указать только различающиеся параметры состояний кнопок
 * @param id ID кнопки, используется, в том числе, в качестве div_id кнопки с состояниями
 * @param actions `actions` для кнопки, игнорируется если есть `states`
 * @param alpha прозрачность кнопки от 0 до 1
 * @param margins внешние отступы
 * @param actionAnimation action_animation в дивах
 * @param animationSpaceSize увеличить расстояние до краев для анимации (изменит общий размер, но иконка и круг
 * останутся прежними)
 * @param defaultStateId
 * @param color
 * @param iconType
 * @param alignment_vertical
 * @param alignment_horizontal
 * @constructor
 */
export function IconButton({
    size,
    iconSize,
    iconUrl,
    background,
    states,
    id,
    actions,
    alpha,
    margins,
    actionAnimation,
    animationSpaceSize = 1,
    defaultStateId,
    color,
    iconType,
    alignment_vertical,
    alignment_horizontal,
}: IIconButtonProps) {
    if (states) {
        const stateDivs: IDivStateBlockState[] = [];

        for (const state of states) {
            const iconElementData: Parameters<typeof IconButtonElement>[0] = {
                size,
                iconUrl: state.iconUrl ?? iconUrl,
                iconSize,
                background: state.background ?? background,
                actions: state.actions,
                alpha: state.alpha ?? alpha,
                actionAnimation,
                color: state.color ?? color,
                iconType,
                alignment_vertical,
                alignment_horizontal,
            };

            if (animationSpaceSize > 1) {
                const calculatedPadding = Math.floor(size * (animationSpaceSize - 1));

                iconElementData.margins = {
                    top: calculatedPadding + (iconElementData.margins?.top || 0),
                    left: calculatedPadding + (iconElementData.margins?.left || 0),
                    bottom: calculatedPadding + (iconElementData.margins?.bottom || 0),
                    right: calculatedPadding + (iconElementData.margins?.right || 0),
                };
            }

            stateDivs.push({
                state_id: state.stateId,
                animation_in: state.animationIn,
                animation_out: state.animationOut,
                div: IconButtonElement(iconElementData),
            });
        }

        const divStateBlockParams: ConstructorParameters<typeof DivStateBlock>[0] = {
            width: new WrapContentSize(),
            height: new WrapContentSize(),
            div_id: id,
            states: stateDivs,
            margins,
            default_state_id: defaultStateId,
            alignment_horizontal,
            alignment_vertical,
        };

        return new DivStateBlock(divStateBlockParams);
    }

    const iconElementData: Parameters<typeof IconButtonElement>[0] = {
        size,
        iconUrl,
        iconSize,
        background,
        actions,
        margins,
        actionAnimation,
        color,
        iconType,
        alignment_vertical,
        alignment_horizontal,
    };

    if (animationSpaceSize > 1) {
        const calculatedPadding = Math.floor(size * (animationSpaceSize - 1));

        iconElementData.margins = {
            top: calculatedPadding + (iconElementData.margins?.top || 0),
            left: calculatedPadding + (iconElementData.margins?.left || 0),
            bottom: calculatedPadding + (iconElementData.margins?.bottom || 0),
            right: calculatedPadding + (iconElementData.margins?.right || 0),
        };
    }

    return IconButtonElement(iconElementData);
}

function IconButtonElement({
    size,
    iconSize,
    iconUrl,
    background,
    actions,
    alpha,
    margins,
    actionAnimation,
    color,
    iconType,
    alignment_vertical,
    alignment_horizontal,
}: Omit<IIconButtonProps, 'states' | 'id'>) {
    return new ContainerBlock({
        width: new FixedSize({ value: size }),
        height: new FixedSize({ value: size }),
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'center',
        alignment_vertical,
        alignment_horizontal,
        margins,
        action_animation: actionAnimation,
        items: [
            Icon({
                iconUrl,
                size: iconSize,
                color,
            }),
        ],
        border: iconType !== 'square' ? {
            corner_radius: size / 2,
        } : undefined,
        background,
        actions,
        alpha,
    });
}

function Icon({
    size,
    iconUrl,
    color,
}: {
    size: number;
    iconUrl: string;
    color?: string;
}) {
    const imageProps: ConstructorParameters<typeof ImageBlock>[0] = {
        width: new FixedSize({ value: size }),
        height: new FixedSize({ value: size }),
        preload_required: 1,
        image_url: iconUrl,
        scale: 'fit',
    };

    if (color) {
        imageProps.tint_color = color;
    }

    return new ImageBlock(imageProps);
}
