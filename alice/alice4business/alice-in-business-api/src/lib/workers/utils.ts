import { sleep } from '../utils';
import log from '../log';
import { Counter, incCounter as solomonIncCounter } from '../../services/solomon';

interface AsyncWorker {
    routine: () => Promise<void>;
    interval: number;
    name: string;
}

type CounterType = 'error' | 'success' | 'total';

const getCounterName = (name: string, counterType: CounterType) => {
    return `worker_${name}_${counterType}`;
};

export class WorkerCounters {
    private intCounters: Counter = {};

    public incCounter = (workerName: string, counterType: CounterType) => {
        const counterName = getCounterName(workerName, counterType);
        if (typeof this.intCounters[counterName] === 'number') {
            solomonIncCounter(counterName, this.intCounters)
        }
    };

    private addCounters = (newCounters: Counter) => {
        this.intCounters = {
            ...this.intCounters,
            ...newCounters,
        };
    };
    public createCounters = (name: string) => {
        if (this.hasCountersFor(name)) {
            return;
        }

        const getName = (type: CounterType) => getCounterName(name, type);

        this.addCounters({
            [getName('total')]: 0,
            [getName('success')]: 0,
            [getName('error')]: 0,
        });
    };
    private hasCountersFor = (name: string) => {
        return Boolean(this.intCounters[getCounterName(name, 'total')]);
    };

    public get counters() {
        return this.intCounters;
    }
}

export const workerCounters = new WorkerCounters();

/**
 * Create async worker.
 * @param routine  Функция обработчик, будет вызываться рекурсивно
 * @param interval {number} Интервал вызова routine
 * @param name {string} Название воркера
 * @return {[() => void, () => void]} Возвращает массив из 2-ух функций start и stop
 */
export const createAsyncWorker = ({ routine, interval, name }: AsyncWorker) => {
    let enable = true;
    workerCounters.createCounters(name);
    const start = async () => {
        try {
            enable = true;
            await routine();
            workerCounters.incCounter(name, 'success');
        } catch (e) {
            workerCounters.incCounter(name, 'error');
            log.error(name + ' worker process error', e);
        } finally {
            workerCounters.incCounter(name, 'total');
            await sleep(interval);
            enable && start();
        }
    };
    const stop = () => void (enable = false);
    return [() => void start(), stop];
};
