import './centaur';
import { Div, TemplateCard } from 'divcard2';
import { templates as centaurTemplates } from './centaur/templates/index';
import { templates as exampleTemplates } from './example/templates';
import { templates as videoTemplates } from './video/templates';
import { templates as onboardingTemplates } from './onboarding/templates';
import { NAlice } from '../protos';
import { MMRequest } from '../common/helpers/MMRequest';
import { IRequestState } from '../common/types/common';
import { ITemplateCard } from './centaur/helpers/helpers';
import { exampleTemplateHelper } from './example';
import { onboardingTemplateHelper } from './onboarding';
import { AllTemplatesType } from '../common/templates/types';

type RenderData = NAlice.NData.TScenarioData;
export type OneOfKey = NonNullable<RenderData['Data']>;
export type RenderFn<T> = (data: T, mmRequest: MMRequest, requestState: IRequestState) => ITemplateCard | TemplateCard<string>;

export type TemplateData = RenderData[OneOfKey];

export type Templates = {
    [K in OneOfKey]?: RenderFn<NonNullable<RenderData[K]>>;
};

export interface IDivPatchElement {
    id: string;
    items: Div[];
}

type RenderFnDivPatch<T> = (data: T, mmRequest: MMRequest, requestState: IRequestState) => IDivPatchElement[];

export type IDivPatchTemplates = {
    [K in OneOfKey]?: RenderFnDivPatch<NonNullable<RenderData[K]>>;
};

export type templateHelperNames = keyof (
    AllTemplatesType &
    typeof exampleTemplateHelper &
    typeof onboardingTemplateHelper
);

export const templates = {
    ...exampleTemplates,
    ...centaurTemplates,
    ...videoTemplates,
    ...onboardingTemplates,
};
