import { Column, CreatedAt, UpdatedAt, DataType, Default, Model, PrimaryKey, Table } from 'sequelize-typescript';

export interface SupportOperationsScheme {
    id: string;
    operationType: string;
    succeed: boolean;
    puid: string;
    createdAt: Date;
    updatedAt: Date;
    message: string | null;
}

@Table({
    tableName: 'support_operations_audit_logs',
    modelName: 'support_operations_audit_log',
    underscored: true,
})
export class SupportOperationsInstance extends Model<SupportOperationsInstance> implements SupportOperationsScheme {
    @PrimaryKey @Default(DataType.UUIDV4) @Column(DataType.UUID) readonly id!: string;
    @Column(DataType.STRING) readonly operationType!: string;
    @Column(DataType.BOOLEAN) readonly succeed!: boolean;
    @Column(DataType.STRING) readonly puid!: string;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
    @Column(DataType.STRING) readonly message!: string | null;

}
