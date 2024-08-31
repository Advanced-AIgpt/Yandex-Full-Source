import { Div } from 'divcard2';
import BasicErrorCard from '../BasicErrorCard';

export default function ErrorCard({ colIndex, rowIndex }: {colIndex: number | undefined; rowIndex: number | undefined }): Div {
    return BasicErrorCard({
        colIndex,
        rowIndex,
        title: 'Ошибка',
        description: 'Что-то пошло не так. Проверьте интернет или обновите экран',
    });
}
