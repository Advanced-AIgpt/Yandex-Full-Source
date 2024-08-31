import { IShareMeta } from './history';
import { IVaultMeta } from './vault';

export interface IJupyterCloudMeta {
    share?: IShareMeta;
    vault?: IVaultMeta;
}
