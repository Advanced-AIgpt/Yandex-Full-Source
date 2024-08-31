import {
    ContainerBlock,
    Div,
    MatchParentSize,
    NonEmptyArray,
} from 'divcard2';
import { simpleBackground } from '../../style/constants';

interface Props {
    children: NonEmptyArray<Div>;
    options?: Partial<ConstructorParameters<typeof ContainerBlock>[0]>;
}

export default function FullScreen({ children, options = {} }: Props) {
    return new ContainerBlock({
        background: simpleBackground,
        height: new MatchParentSize(),
        // height: new FixedSize({ value: 800 }),
        width: new MatchParentSize(),
        items: children,
        ...options,
    });
}
