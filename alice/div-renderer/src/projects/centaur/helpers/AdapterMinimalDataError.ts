export class AdapterMinimalDataError extends Error {
    constructor(missedData: string[], template: string) {
        super(`Missed data (${missedData.join(', ')}) in template ${template}`);
        this.name = 'AdapterMinimalDataError';
    }
}
