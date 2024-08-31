export type Days = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday'];
export type Day = Days[number];

export const days: Days = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday'];

export const dayLabels: { [k in Day]: string } = {
    Monday: 'Пн',
    Tuesday: 'Вт',
    Wednesday: 'Ср',
    Thursday: 'Чт',
    Friday: 'Пт',
    Saturday: 'Сб',
    Sunday: 'Вс',
};

export interface DayOpeningHours {
    from: string;
    to: string;
}

export type WeekOpeningHours = { [k in Day]: DayOpeningHours | null };

export const defaultDayOpeningHours = {
    from: '10:00',
    to: '18:00',
};

export const defaultOpeningHours = {
    Monday: defaultDayOpeningHours,
    Tuesday: defaultDayOpeningHours,
    Wednesday: defaultDayOpeningHours,
    Thursday: defaultDayOpeningHours,
    Friday: defaultDayOpeningHours,
    Saturday: null,
    Sunday: null,
};

export interface Timezone {
    name: string;
    offset: number;
    title: string;
}
