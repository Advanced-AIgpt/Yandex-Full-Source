all: clean bundle
bundle:: worker
	cat recorder.js recorderWorker.js recognizer.js speechrecognition.js tts.js webaudiowrapper.js internal/speakerid.js ui/*.js > build/webspeechkit.js
worker::
	sed -e 's/\(.*\)/"\1" +/g' recorderWorkerSrc.js > tmp.js
	echo '" ";' >> tmp.js
	sed -e "/var inline_worker =/r tmp.js" recorderWorkerCreator.js > recorderWorker.js
	rm -f tmp.js
clean::
	rm -f tmp.js
	rm -f build/webspeechkit.js
