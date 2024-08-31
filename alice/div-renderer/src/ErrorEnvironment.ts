export class ErrorEnvironment extends Error {
    constructor(envName: string) {
        super(`Environment variable ${envName} must be defined`);
        this.name = 'ErrorEnvironment';
    }
}
