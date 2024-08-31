import { Div, IDivAction, IDivTrigger } from 'divcard2';
import { IRequestState, LocalTemplatesRegistryType } from '../common/types/common';
import TemplateRegistry from './templateRegistry';
import { MMRequest } from '../common/helpers/MMRequest';
import { hasExperiment } from '../projects/centaur/expFlags';
import { isEqual } from 'lodash';
import { LocalTemplateRegistry, GlobalTemplateRegistry } from '../common/helpers/createTemplate';
import { AnalyticsContext } from '../common/analytics/context';

const fixScale = 1.063;

export function addVariableTrigger(trigger: IDivTrigger, requestState: IRequestState) {
    requestState.variableTriggers.add(trigger);
}

class ActionSet {
    private items: IDivAction[] = [];

    add(action: IDivAction) {
        const existsAction = this.items.find(el => isEqual(el, action));
        if (!existsAction) {
            this.items.push(action);
        }
        return this;
    }

    getAll(): IDivAction[] {
        return this.items;
    }
}

export class TriggerSet {
    private items: (Omit<IDivTrigger, 'actions'> & { actions: ActionSet })[] = [];

    add(triggers: IDivTrigger | IDivTrigger[]) {
        if (!Array.isArray(triggers)) {
            triggers = [triggers];
        }

        triggers.forEach(trigger => {
            const id = trigger.condition as string;

            const existsTrigger = this.items.find(el => el.condition === id);

            if (!existsTrigger) {
                const actions = new ActionSet();

                for (const action of (trigger.actions ?? []) as IDivAction[]) {
                    actions.add(action);
                }

                this.items.push({
                    condition: trigger.condition,
                    actions,
                });
            } else {
                if (trigger.actions) {
                    (trigger.actions as IDivAction[]).forEach(el => existsTrigger.actions.add(el));
                }
            }
        });

        return this;
    }

    getAll(): IDivTrigger[] {
        return this.items.map(el => {
            return {
                ...el,
                actions: el.actions.getAll(),
            };
        });
    }
}

export function createRequestState(mmRequest?: MMRequest): IRequestState {
    const deviceWidth = 1280;
    const deviceHeight = 800;

    return {
        res: {
            localTemplates: new LocalTemplateRegistry(),
            globalTemplates: new GlobalTemplateRegistry(),
        },
        globalTemplates: new Set(),
        localTemplates: new Set(),
        variableTriggers: new TriggerSet(),
        variables: new Set(),
        sizes: {
            width: deviceWidth * fixScale,
            height: deviceHeight * fixScale,
        },
        hasExperiment: exp => {
            if (mmRequest) {
                return hasExperiment(mmRequest, exp);
            }
            return false;
        },
        analyticsContext: AnalyticsContext.startWithReqId(mmRequest?.RequestId),
    };
}

export function mergeRequestState(basis: IRequestState, moreState: IRequestState) {
    moreState.localTemplates.forEach(item => basis.localTemplates.add(item));
    moreState.globalTemplates.forEach(item => basis.globalTemplates.add(item));
    moreState.variables.forEach(item => basis.variables.add(item));
    basis.variableTriggers.add(moreState.variableTriggers.getAll());
    return basis;
}

export const templatesRegistry = new TemplateRegistry();

export function getTemplates(localRegistry: LocalTemplatesRegistryType, globalRegistry: TemplateRegistry) {
    const templates: { [name: string]: Div } = {};
    for (const id of localRegistry) {
        const templateItem = globalRegistry.get(id);
        if (templateItem !== null) {
            templates[id] = templateItem;
        }
    }

    return templates;
}
