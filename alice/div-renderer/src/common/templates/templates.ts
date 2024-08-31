import { Div, TemplateBlock } from 'divcard2';
import { IRequestState } from '../types/common';
import { mergeRequestState } from '../../registries/common';
import { AllTemplatesType } from './types';
import { templateHelperNames } from '../../projects';

type AllTemplatesData = {[name in keyof AllTemplatesType]: unknown};

type ITemplatesType = {[name: string]: [Div, IRequestState]};

export class TemplatesHelperClass<T extends AllTemplatesData> {
    private templates: ITemplatesType = {};

    public add(templateName: keyof AllTemplatesType, template: [Div, IRequestState]) {
        if (Object.prototype.hasOwnProperty.call(this.templates, templateName)) {
            throw new Error(`Div templates must be uniq. Duplicated template is '${templateName}'.`);
        }

        this.templates[templateName] = template;

        return this;
    }

    public use<EXC extends string, K extends keyof T>(
        templateName: K,
        options: Omit<T[K], EXC>,
        requestState: IRequestState,
        type: 'local' | 'global' = 'local',
    ) {
        if (!Object.prototype.hasOwnProperty.call(this.templates, templateName)) {
            throw new Error(`Template '${templateName}' is not defined.`);
        }

        const [, templateRequestState] = this.templates[templateName as templateHelperNames];

        mergeRequestState(requestState, templateRequestState);

        switch (type) {
            case 'local':
                requestState.localTemplates.add(templateName as templateHelperNames);
                break;
            case 'global':
                requestState.globalTemplates.add(templateName as templateHelperNames);
                break;
        }

        return new TemplateBlock(templateName as templateHelperNames, options);
    }

    public getUsedTemplates(requestState: IRequestState) {
        const templates: {[name: string]: Div} = {};
        for (const key of requestState.localTemplates) {
            if (Object.prototype.hasOwnProperty.call(this.templates, key)) {
                templates[key] = this.templates[key][0];
            }
        }
        return templates;
    }
}
