//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsExistingWorkPolicy](index.md)

# PassportContactsExistingWorkPolicy

[passport-contacts]\
enum [PassportContactsExistingWorkPolicy](index.md) : Enum&lt;[PassportContactsExistingWorkPolicy](index.md)&gt; 

Defines how to behave when sync work was already scheduled before.

## Entries

| | |
|---|---|
| [APPEND](-a-p-p-e-n-d/index.md) | [passport-contacts]<br>[APPEND](-a-p-p-e-n-d/index.md)(ExistingWorkPolicy.APPEND, ExistingPeriodicWorkPolicy.REPLACE)<br>If there is existing pending (uncompleted) work with the same unique name, append the newly-specified work as a child of all the leaves of that work sequence.  Otherwise, insert the newly-specified work as the start of a new sequence. <br/><b>Note:</b> When using APPEND with failed or cancelled prerequisites, newly enqueued work will also be marked as failed or cancelled respectively. Use {@link ExistingWorkPolicy#APPEND_OR_REPLACE} to create a new chain of work. |
| [KEEP](-k-e-e-p/index.md) | [passport-contacts]<br>[KEEP](-k-e-e-p/index.md)(ExistingWorkPolicy.KEEP, ExistingPeriodicWorkPolicy.KEEP)<br>If there is existing pending (uncompleted) work with the same unique name, do nothing. Otherwise, insert the newly-specified work. |
| [REPLACE](-r-e-p-l-a-c-e/index.md) | [passport-contacts]<br>[REPLACE](-r-e-p-l-a-c-e/index.md)(ExistingWorkPolicy.REPLACE, ExistingPeriodicWorkPolicy.REPLACE)<br>If there is existing pending (uncompleted) work with the same unique name, cancel and delete it.  Then, insert the newly-specified work. |
