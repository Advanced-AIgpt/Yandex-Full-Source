/* eslint-disable camelcase */
import { sortedIndexBy } from 'lodash';
import { format, isSameDay } from 'date-fns';
import { ru } from 'date-fns/locale';
import { Div, IDivData, TextBlock } from 'divcard2';
import { IRequestState } from '../../../common/types/common';
import { getTemplates, templatesRegistry } from '../../../registries/common';
import { centaurTemplatesClass } from '../index';
import { Palette } from './palette';

export { default as pluralize } from '@yandex-int/i18n/lib/plural/ru';
type TextProps = Pick<ConstructorParameters<typeof TextBlock>[0], 'text_color'|'font_size'|'font_weight'|'line_height'>;

export const textBreakpoint =
    (...breakpoints: [number, TextProps][]) =>
        (text: string) => {
            const idx = Math.min(sortedIndexBy(breakpoints, [text.length, {}], 0), breakpoints.length - 1);
            return { text, ...breakpoints[idx][1] };
        };

export const formatDate = (d: Date | string | number, f: string) => format(new Date(d), f, { locale: ru });

export const textCommand = (c: string) => `dialog://text_command?query=${encodeURI(c)}`;

export const updatedAt = (d: Date) => {
    const now = new Date();
    return `Обновлен ${isSameDay(now, d) ? 'сегодня' : formatDate(d, 'dd.MM')}`;
};

export const insertBetween = <I extends unknown, S extends unknown>(itemList: I[], separator: S) => {
    return itemList.reduce<(I | S)[]>((result, item, index) => {
        if (index !== 0) {
            result.push(separator);
        }

        result.push(item);

        return result;
    }, []);
};

export interface ITemplateCard {
    templates: {
        [name: string]: Div;
    };
    palette?: Palette;
    card: IDivData;
}

// Дива верхнего уровня. Замена для new `TemplateCard()`
export const TopLevelCard = (card: Omit<IDivData, 'variable_triggers' | 'variables'>, requestState: IRequestState, palette?: Palette) => {
    const templates = Object.assign(
        getTemplates(requestState.localTemplates, templatesRegistry),
        centaurTemplatesClass.getUsedTemplates(requestState),
        requestState.res.localTemplates.getAll(),
    );

    return {
        templates,
        palette,
        card: {
            ...card,
            variable_triggers: requestState.variableTriggers.getAll(),
            variables: [...requestState.variables],
        } as IDivData,
    };
};
