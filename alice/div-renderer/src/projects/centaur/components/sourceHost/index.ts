import { TextBlock, WrapContentSize } from 'divcard2';
import { title32m } from '../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../style/constants';

export function sourceHost(host: string, options: Partial<ConstructorParameters<typeof TextBlock>[0]> = {}) {
    host = host.replace(/^(https?:)?\/\//, '');

    return new TextBlock({
        ...title32m,
        text_color: colorWhiteOpacity50,
        text: host,
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        ...options,
    });
}
