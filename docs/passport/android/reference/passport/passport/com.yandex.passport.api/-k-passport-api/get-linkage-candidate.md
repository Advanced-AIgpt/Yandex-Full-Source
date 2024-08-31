//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getLinkageCandidate](get-linkage-candidate.md)

# getLinkageCandidate

[passport]\
abstract suspend fun [getLinkageCandidate](get-linkage-candidate.md)(uid: [PassportUid](../-passport-uid/index.md)): Result&lt;[PassportAccount](../-passport-account/index.md)?&gt;

Возвращает аккаунт, подходящий для связывания с указанным аккаунтом.

Положительный ответ возможен при соблюдении большого количества условий, которые могут регулярно меняться: свойства аккаунтов, их количество, ответ бэкенда, количество уже сделанных отказов и их частота, и так далее. Возможная пара кандидатов валидируется через запрос в бэкенд внутри SyncAdapter-а, то есть выполняется в фоновом режиме (это может измениться в будущем).

Если кандидат найден, можно показать соответствующий интерфейс с предложением связать аккаунты. Выбор подходящего момента зависит от вас. Пользователь должен иметь возможность не только согласиться со связыванием, но и отказаться от него. При положительном решении необходимо вызвать метод [performLinkageForce](perform-linkage-force.md),

#### Return

Объект [PassportAccount](../-passport-account/index.md), если кандидат для связывания найден, или null, если кандидата нет.

## See also

passport

| | |
|---|---|
|  | .performLinkageForce |

## Parameters

passport

| | |
|---|---|
| uid | Идентификатор [PassportUid](../-passport-uid/index.md) аккаунта, для которого нужно найти кандидата для связывания. |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | Выбрасывается, если аккаунт с указанным uid-ом не найден. |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | Выбрасывается во всех остальных непредвиденных случаях. |
