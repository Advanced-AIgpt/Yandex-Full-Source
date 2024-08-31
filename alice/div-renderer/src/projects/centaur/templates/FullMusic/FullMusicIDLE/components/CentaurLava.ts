import { DivCustomBlock, MatchParentSize } from 'divcard2';

interface Props {
    fpsPosition?: {
        x: number;
        y: number;
    };
}

export default function CentaurLava({ fpsPosition = { x: 0.5, y: 0.01 } }: Props = {}) {
    return new DivCustomBlock({
        custom_type: 'centaur_lava',
        custom_props: {
            fpsPosition,
        },
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        extensions: [
            {
                id: 'centaur-audio-lava',
            },
        ],
    });
}
