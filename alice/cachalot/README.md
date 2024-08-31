Документация: https://wiki.yandex-team.ru/voiceinfra/services/cachalot/

Для конвертации бинарного лога в читаемый формат использовать
./cachalot evlogdump $LOG_DIR/cachalot.evlog

Для конвертации бинарного RT лога (см. server/rtlog в dev_config.json) в читаемый формат использовать
arcadia/alice/rtlog/evlogdump/evlogdump cachalot.rtlog

Для переоткрытия (после ротации) логфайла
curl localhost:$cachalot_http_port/admin?action=reopenlog

