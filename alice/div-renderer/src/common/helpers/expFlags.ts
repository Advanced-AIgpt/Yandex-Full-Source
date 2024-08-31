import { MMRequest } from './MMRequest';

export function createHasExperiment<ExpFlags extends string>() {
    return function hasExperiment(mmRequest: MMRequest, experiment: ExpFlags): boolean {
        if (mmRequest?.Experiments?.fields) {
            return Object.prototype.hasOwnProperty.call(mmRequest.Experiments.fields, experiment);
        }

        return false;
    };
}
