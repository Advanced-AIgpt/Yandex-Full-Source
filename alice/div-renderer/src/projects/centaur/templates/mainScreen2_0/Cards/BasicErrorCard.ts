import { Div, TextBlock } from 'divcard2';
import { BasicTextCard } from './BasicTextCard';
import { text28m } from '../../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../../style/constants';
import { getCardMainScreenId } from './helpers';

interface Props {
    title: string;
    description: string;
    actions?: Parameters<typeof BasicTextCard>[0]['actions'];
    longtap_actions?: Parameters<typeof BasicTextCard>[0]['longtap_actions'];
    rowIndex: number | undefined;
    colIndex: number | undefined;
}

export default function BasicErrorCard({ title, description, actions, longtap_actions, colIndex, rowIndex }: Props): Div {
    return BasicTextCard({
        id: getCardMainScreenId({ colIndex, rowIndex }),
        title,
        actions,
        longtap_actions,
        items: [
            new TextBlock({
                ...text28m,
                text_color: colorWhiteOpacity50,
                text: description,
            }),
        ],
    });
}
