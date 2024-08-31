import { FixedSize, IDivAction, ImageBlock, SolidBackground, Template } from 'divcard2';
import { compact } from 'lodash';
import { colorWhiteOpacity10 } from '../../style/constants';
import { closeLayerLocalAction } from '../../actions/client';
import { getStaticS3Asset } from '../../helpers/assets';
import { Layer } from '../../common/layers';

interface Props {
    size?: Readonly<number>;
    padding?: Readonly<number>;
    actions?: IDivAction[];
    options?: Readonly<Partial<ConstructorParameters<typeof ImageBlock>[0]>>;
    layer?: Layer;
    preventDefault?: boolean;
    backgroundColor?: string;
    borderRadius?: number;
}

export const closeButtonDefaultSize = 88;

export const defaultActionsCloseLayer: Readonly<IDivAction[]> = [
    {
        log_id: 'close_fullscreen',
        url: closeLayerLocalAction(),
    },
];

export default function CloseButton({
    size = closeButtonDefaultSize,
    padding = 22,
    options = {},
    layer = Layer.DIALOG,
    preventDefault = false,
    backgroundColor = colorWhiteOpacity10,
    borderRadius = 20,
}: Props = {}) {
    const { actions, ...otherOptions } = options;

    const real_actions = (actions && actions instanceof Template) ?
        options.actions :
        compact([
            ...(actions || []),
            !preventDefault ?
                {
                    log_id: 'close_fullscreen',
                    url: closeLayerLocalAction(layer),
                } : null,
        ]);

    return new ImageBlock({
        image_url: getStaticS3Asset('icons/close_wt_border.png'),
        width: new FixedSize({ value: size }),
        height: new FixedSize({ value: size }),
        scale: 'fill',
        background: [new SolidBackground({
            color: backgroundColor,
        })],
        paddings: {
            top: padding,
            right: padding,
            left: padding,
            bottom: padding,
        },
        actions: real_actions,
        border: {
            corner_radius: borderRadius,
        },
        preload_required: 1,
        alignment_horizontal: 'right',
        ...otherOptions,
    });
}
