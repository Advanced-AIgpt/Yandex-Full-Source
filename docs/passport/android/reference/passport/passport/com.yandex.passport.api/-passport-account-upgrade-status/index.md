//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccountUpgradeStatus](index.md)

# PassportAccountUpgradeStatus

[passport]\
enum [PassportAccountUpgradeStatus](index.md) : Enum&lt;[PassportAccountUpgradeStatus](index.md)&gt; 

Represents current upgrade status of give account.

## Entries

| | |
|---|---|
| [REQUIRED](-r-e-q-u-i-r-e-d/index.md) | [passport]<br>[REQUIRED](-r-e-q-u-i-r-e-d/index.md)()<br>Upgrade is required and cannot be skipped. |
| [SKIPPED](-s-k-i-p-p-e-d/index.md) | [passport]<br>[SKIPPED](-s-k-i-p-p-e-d/index.md)()<br>Upgrade was temporary skipped by user. But will be needed later. |
| [NEEDED](-n-e-e-d-e-d/index.md) | [passport]<br>[NEEDED](-n-e-e-d-e-d/index.md)()<br>Account is needed to be upgraded. Upgrade can be temporary skipped by user. |
| [NOT_NEEDED](-n-o-t_-n-e-e-d-e-d/index.md) | [passport]<br>[NOT_NEEDED](-n-o-t_-n-e-e-d-e-d/index.md)()<br>Account is already fully upgraded or upgrade is not applicable at the moment. |

## Extensions

| Name | Summary |
|---|---|
| [canAccountBeUpgraded](../can-account-be-upgraded.md) | [passport]<br>val [PassportAccountUpgradeStatus](index.md).[canAccountBeUpgraded](../can-account-be-upgraded.md): Boolean |
| [shouldAccountBeUpgraded](../should-account-be-upgraded.md) | [passport]<br>val [PassportAccountUpgradeStatus](index.md).[shouldAccountBeUpgraded](../should-account-be-upgraded.md): Boolean |
