.PHONY: test gettext initial_copy_po update_po compile_mo translate_po

# Команды для работы с переводами
# Для обновления переводов нужно:
#
# LANGUAGE=en make gettext
# LANGUAGE=en make update_po
# LANGUAGE=en make translate_po
# посмотреть глазами дифф, поправить лишние переносы строк
# LANGUAGE=en make compile_mo

LANGUAGE ?= en
TRANSLATION_FILES = generic_scenario_to_human_readable.py visualize_alarms_timers.py visualize_directive.py visualize_geo.py visualize_iot.py visualize_multiroom.py visualize_music.py visualize_open_uri.py visualize_state.py visualize_video.py

# проверка работы переводов через задание env LANGUAGE=ar
test:
	LANGUAGE=ru python test_i18n.py
	LANGUAGE=ar python test_i18n.py
	LANGUAGE=en python test_i18n.py

# выгрузка из py файлов в pot
gettext:
	for file in ${TRANSLATION_FILES} ; do \
		xgettext -d $${file%%.*} -o i18n/$${file%%.*}.pot $${file}; \
	done

# первоначально скопировать pot в po файлы
initial_copy_po:
	mkdir -p i18n/${LANGUAGE}/LC_MESSAGES
	for file in ${TRANSLATION_FILES} ; do \
    	cp i18n/$${file%%.*}.pot i18n/${LANGUAGE}/LC_MESSAGES/$${file%%.*}.po; \
	done

# обновление po файлов из перегенерённых pot
update_po:
	for file in ${TRANSLATION_FILES} ; do \
    	msgmerge --update i18n/${LANGUAGE}/LC_MESSAGES/$${file%%.*}.po i18n/$${file%%.*}.pot; \
	done

# прогоняет через автопереводчик
translate_po:
	for file in ${TRANSLATION_FILES} ; do \
    	python3 i18n/translate_po_yandex.py i18n/${LANGUAGE}/LC_MESSAGES/$${file%%.*}.po; \
	done

# компилирует mo файлы из переведённых po файлов
compile_mo:
	for file in ${TRANSLATION_FILES} ; do \
    	msgfmt -o i18n/${LANGUAGE}/LC_MESSAGES/$${file%%.*}.mo i18n/${LANGUAGE}/LC_MESSAGES/$${file%%.*}.po; \
	done
