import 'dotenv/config';
import { ErrorEnvironment } from './ErrorEnvironment';

export function getEnvs() {
    const { NODE_ENV } = process.env;

    if (!process.env.SECRET_MAPS_SIGNING_SECRET) {
        throw new ErrorEnvironment('SECRET_MAPS_SIGNING_SECRET');
    }
    if (!process.env.SECRET_MAPS_API_KEY) {
        throw new ErrorEnvironment('SECRET_MAPS_API_KEY');
    }
    return {
        PORT: Number(process.env.PORT) || 10000,
        SOLOMON_PORT: Number(process.env.SOLOMON_PORT) || 8000,
        SECRET_MAPS_SIGNING_SECRET: process.env.SECRET_MAPS_SIGNING_SECRET,
        SECRET_MAPS_API_KEY: process.env.SECRET_MAPS_API_KEY,
        isProd: NODE_ENV === 'production',
        isDev: NODE_ENV === 'development',
    };
}
