import { SeparatorBlock } from 'divcard2';
import { colorWhiteOpacity0 } from '../../style/constants';

/**
 * Класс пустого дива, в библиотеке дивов пустые блоки как таковые отсутствуют и рекомендуют использовать
 * прозрачный сепаратор или текст с пробелом. По сути сейчас это алиас для сепаратора с заранее заданным
 * прозрачным delimiter_style
 */
export default class EmptyDiv extends SeparatorBlock {
    constructor(props?: Omit<ConstructorParameters<typeof SeparatorBlock>[0], 'delimiter_style'>) {
        super({
            ...props,
            delimiter_style: {
                color: colorWhiteOpacity0,
            },
        });
    }
}
