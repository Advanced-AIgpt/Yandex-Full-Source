import {
    Column,
    CreatedAt,
    DataType,
    Default,
    Model,
    PrimaryKey,
    Table,
    UpdatedAt,
} from 'sequelize-typescript';
import { PlusResponseStatus } from '../../services/media/ya-plus';
import { OperationInstance } from './operation';

export interface PromoCodeSchema {
    id: string;
    organizationId: string;
    code: string;
    userId: string | null;
    status: PlusResponseStatus | 'pending' | null;
    operationId: string | null;
    ticketKey: string;

    createdAt: Date;
    updatedAt: Date;
}

@Table({
    tableName: 'promo_codes',
    modelName: 'promoCode',
    underscored: true,
})
export class PromoCodeInstance extends Model<PromoCodeInstance>
    implements PromoCodeSchema {
    @PrimaryKey @Default(DataType.UUIDV4) @Column(DataType.UUID) readonly id!: string;
    @Column(DataType.STRING) readonly code!: string;
    @Column(DataType.STRING) readonly userId!: string | null;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
    @Column(DataType.UUID) readonly organizationId!: string;
    @Column(DataType.TEXT) readonly status!: PlusResponseStatus | 'pending' | null;
    @Column(DataType.UUID) readonly operationId!: string | null;
    @Column(DataType.STRING) readonly ticketKey!: string;

    readonly getOperation!: () => Promise<OperationInstance>;
}
