import { Div, Template, TemplateBlock } from 'divcard2';
import { memoize } from 'lodash';
import AbstractRegistry from '../../registries/abstractRegistry';
import { IRequestState } from '../types/common';

export type SomeTemplateProps<P> = Record<keyof P, Template>;
export type SomeTemplate<P> = (props: SomeTemplateProps<P>) => Div;
type SomeTemplateType = 'local' | 'global';

const getPlaceholderProps = <P extends object>(props: P) => {
    const propsKeys = Object.keys(props) as (keyof P)[];
    const templateProps = propsKeys.reduce<SomeTemplateProps<P>>((result, key) => {
        result[key] = new Template(String(key));

        return result;
    }, {} as SomeTemplateProps<P>);

    return templateProps;
};

export const buildTemplateFabric = (type: SomeTemplateType) => <P extends object>(template: SomeTemplate<P>) => {
    const templateName = `${type}_template_${template.name}_${Math.random()}`;
    /**
     * В теории, пропсов при вызове темплейта быть не должно. Темплейт должен всегда возвращать один и тот же результат.
     * Но на практке, данная реализация темплейта подразумевает передачу пропсов, поскольку они используются для
     * генерации на их основе плейсхолдеров для данных. Использование плейсхолдеров позволяет как типизировать пропсы на входе,
     * так и типизировать их использование внутри темплейта.
     * Поэтому, для подсчета ключа кэширования важны не столько значения полей пропсов, сколько сам их набор.
     */
    const templateToExec = memoize(template, props => Object.keys(props).join(','));

    return (props: P, requestState: IRequestState) => {
        const templateProps = getPlaceholderProps<P>(props);
        const templateResult = templateToExec(templateProps);

        const registry = (() => {
            switch (type) {
                case 'local': return requestState.res.localTemplates;
                case 'global': return requestState.res.globalTemplates;
            }
        })();

        registry.addOrReplace(templateName, templateResult);

        return new TemplateBlock(templateName, props);
    };
};

export const createLocalTemplate = buildTemplateFabric('local');
export const createGlobalTemplate = buildTemplateFabric('global');

export class LocalTemplateRegistry extends AbstractRegistry<Div> {}
export class GlobalTemplateRegistry extends AbstractRegistry<Div> {}
