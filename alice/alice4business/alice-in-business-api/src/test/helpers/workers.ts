export const startWorkers = async (worker: (() => undefined)[], units: number = 10) => {
    while (units-- > 0) {
        worker[0]();
    }
};

export const stopWorkers = async (worker: (() => undefined)[]) => {
    worker[1]();
};
