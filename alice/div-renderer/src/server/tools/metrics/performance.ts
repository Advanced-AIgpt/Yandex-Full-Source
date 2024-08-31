import { setRegistry, RegistryInterface, startGCMetricCollector, startEventLoopLagMetricCollector } from '@yandex-int/node-metrics';
import { incGCRun, putTimerToEventLoopRun, putTimeToGCRun } from '../../../solomon';

const gcPrefix = 'gc';
const eventLoopPrefix = 'el';

const parseEventName = (name: string) => {
    const [prefix, type] = name.split('_');
    const formattedName = `${prefix}_${type}`;

    return { prefix, formattedName };
};

const registry: RegistryInterface = {
    incrCounter(name) {
        const { prefix, formattedName } = parseEventName(name);

        switch (prefix) {
            case gcPrefix: {
                incGCRun(formattedName);
            }
        }

        return this;
    },
    updateHistogram(name, value) {
        const { prefix, formattedName } = parseEventName(name);

        switch (prefix) {
            case gcPrefix: {
                putTimeToGCRun(formattedName, value);

                break;
            }
            case eventLoopPrefix: {
                putTimerToEventLoopRun(value);

                break;
            }
        }

        return this;
    },
    updateGauge() {
        return this;
    },
};

export const startHandlePerformanceMetrics = () => {
    setRegistry(registry);

    startGCMetricCollector(gcPrefix);
    startEventLoopLagMetricCollector(eventLoopPrefix);
};
