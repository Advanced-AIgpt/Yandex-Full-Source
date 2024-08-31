//[passport-contacts](../../../../index.md)/[com.yandex.passport.contacts](../../index.md)/[PassportContactsExistingWorkPolicy](../index.md)/[APPEND](index.md)

# APPEND

[passport-contacts]\
[APPEND](index.md)(ExistingWorkPolicy.APPEND, ExistingPeriodicWorkPolicy.REPLACE)

If there is existing pending (uncompleted) work with the same unique name, append the newly-specified work as a child of all the leaves of that work sequence.  Otherwise, insert the newly-specified work as the start of a new sequence. <br/><b>Note:</b> When using APPEND with failed or cancelled prerequisites, newly enqueued work will also be marked as failed or cancelled respectively. Use {@link ExistingWorkPolicy#APPEND_OR_REPLACE} to create a new chain of work.
