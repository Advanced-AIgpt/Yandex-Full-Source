import {
    Div,
} from 'divcard2';
import FullScreen from '../../../components/FullScreen';
import TitleLine from '../../../components/TitleLine/TitleLine';
import {
    gradientToBlackTop,
} from '../../../style/constants';

export const DeviceName = 'SmartSpeaker';

interface Props {
    children: Readonly<Div[]>;
}

export default function TeaserSettingsWrapper({ children }: Props) {
    const name = 'Настройка тизеров';

    return FullScreen({
        children: [
            ...children,
            TitleLine({
                title: name,
                options: { background: gradientToBlackTop },
            }),
        ],
        options: {
            orientation: 'overlap',
        },
    });
}
