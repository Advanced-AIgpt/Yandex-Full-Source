import { CounterType } from './types';
import { Counter, incCounter as solomonIncCounter } from '../solomon';

export const getCounterName = (sourceName: string, counterType: CounterType) => {
    return `http_source_${sourceName}_requests_${counterType}`;
};

export class GotCounters {
    private intCounters: Counter = {};

    public incCounter = (sourceName: string, counterType: CounterType) => {
        const counterName = getCounterName(sourceName, counterType);

        if (typeof this.intCounters[counterName] === 'number') {
            solomonIncCounter(counterName, this.intCounters);
        }
    };

    public createCounters = (sourceName: string) => {
        if (this.hasCountersFor(sourceName)) {
            return;
        }

        const getName = (type: CounterType) => getCounterName(sourceName, type);

        this.addCounters({
            [getName('total')]: 0,
            [getName('2xx')]: 0,
            [getName('3xx')]: 0,
            [getName('4xx')]: 0,
            [getName('5xx')]: 0,
            [getName('error')]: 0,
            [getName('connection_error')]: 0,
            [getName('timeout_error')]: 0,
            [getName('other_error')]: 0,
        });
    };

    private addCounters = (newCounters: Counter) => {
        this.intCounters = {
            ...this.intCounters,
            ...newCounters,
        };
    };

    private hasCountersFor = (sourceName: string) => {
        return Boolean(this.intCounters[getCounterName(sourceName, 'total')]);
    };

    public get counters() {
        return this.intCounters;
    }
}

const gotCounters = new GotCounters();

export default gotCounters;
