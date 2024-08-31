//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportEnvironment](../index.md)/[Factory](index.md)/[from](from.md)

# from

[passport]\
fun [from](from.md)(integer: Int): [PassportEnvironment](../index.md)

Фабричный метод, возвращающий ненулевой объект паспортного окружения [PassportEnvironment](../index.md), соответствующего заданному числу. Если передано неизвестное значение, будет возвращён объект боевого внешнего окружения [Passport.PASSPORT_ENVIRONMENT_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md). Это разумный fallback без выбрасывания исключений для прямой совместимости - если в будущих версиях появится новое окружение, то старые версии хотя бы не споткнутся и возможно смогут нормально работать (но это не точно).

#### Return

Ненулевой объект паспортного окружения [PassportEnvironment](../index.md).

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportEnvironment](../index.md) |  |
| PassportEnvironment.getInteger |  |
| [com.yandex.passport.api.Passport](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md) |  |

## Parameters

passport

| | |
|---|---|
| integer | Уникальное число, однозначно идентифицирующее паспортное окружение. Может быть получено с помощью PassportEnvironment.getInteger. |
