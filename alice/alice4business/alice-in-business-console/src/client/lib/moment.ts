import moment from 'moment';

moment.locale('ru', {
    calendar: {
        lastDay: '[Вчера] YYYY.MM.DD',
        sameDay: '[Сегодня] YYYY.MM.DD',
        sameElse: 'YYYY.MM.DD',
        nextDay: 'YYYY.MM.DD',
        nextWeek: 'YYYY.MM.DD',
        lastWeek: 'YYYY.MM.DD',
    },
});

export default moment;
