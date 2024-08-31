//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportEnvironment](../index.md)/[Factory](index.md)

# Factory

[passport]\
object [Factory](index.md)

## Functions

| Name | Summary |
|---|---|
| [from](from.md) | [passport]<br>fun [from](from.md)(integer: Int): [PassportEnvironment](../index.md)<br>Фабричный метод, возвращающий ненулевой объект паспортного окружения [PassportEnvironment](../index.md), соответствующего заданному числу. Если передано неизвестное значение, будет возвращён объект боевого внешнего окружения [Passport.PASSPORT_ENVIRONMENT_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md). Это разумный fallback без выбрасывания исключений для прямой совместимости - если в будущих версиях появится новое окружение, то старые версии хотя бы не споткнутся и возможно смогут нормально работать (но это не точно). |
