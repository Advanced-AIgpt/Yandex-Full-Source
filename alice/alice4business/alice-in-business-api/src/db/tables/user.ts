import {
    Column,
    CreatedAt,
    DataType,
    Model,
    PrimaryKey,
    Table,
    UpdatedAt,
} from 'sequelize-typescript';

export interface UserSchema {
    id: string;
    login: string;

    createdAt: Date;
    updatedAt: Date;
}

@Table({
    tableName: 'users',
    modelName: 'user',
    underscored: true,
})
export class UserInstance extends Model<UserInstance> implements UserSchema {
    @PrimaryKey @Column(DataType.TEXT) readonly id!: string;
    @Column(DataType.TEXT) readonly login!: string;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
}
