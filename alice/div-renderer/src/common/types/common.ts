import { Div, DivVariable, IDivData } from 'divcard2';
import { NAlice } from '../../protos';
import { MMRequest } from '../helpers/MMRequest';
import { hasExperiment } from '../../projects/centaur/expFlags';
import { templateHelperNames } from '../../projects';
import { TriggerSet } from '../../registries/common';
import AbstractRegistry from '../../registries/abstractRegistry';
import { AnalyticsContext } from '../analytics/context';

export type LocalTemplatesRegistryType = Set<templateHelperNames>;

export interface IRequestState {
    globalTemplates: LocalTemplatesRegistryType;
    localTemplates: LocalTemplatesRegistryType;
    variableTriggers: TriggerSet;
    variables: Set<DivVariable>;
    sizes: {
        width: number;
        height: number;
    };
    hasExperiment: (exp: Parameters<typeof hasExperiment>[1]) => ReturnType<typeof hasExperiment>;
    res: {
        localTemplates: AbstractRegistry<Div>
        globalTemplates: AbstractRegistry<Div>;
    }
    analyticsContext: AnalyticsContext;
}

export type TTopLevelDivFunction = (
    data: NAlice.NData.ITCentaurMainScreenMusicTabData,
    mmRequest: MMRequest,
    requestState: IRequestState,
) => {templates: {[p: string]: Div}, card: IDivData};
