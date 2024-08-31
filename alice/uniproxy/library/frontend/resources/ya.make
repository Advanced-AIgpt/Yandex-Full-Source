PY3_LIBRARY()
OWNER(g:voicetech-infra)

RESOURCE(
    analyser.js /frontend/analyser.js
    demo.html /frontend/demo.html
    demo.js /frontend/demo.js
    mic0.png /frontend/mic0.png
    mic1.png /frontend/mic1.png
    robots.txt /frontend/robots.txt
    service_worker.js /frontend/service_worker.js
    settings.js /frontend/settings.js
    spk0.png /frontend/spk0.png
    spk1.png /frontend/spk1.png
    ttsdemo.html /frontend/ttsdemo.html
    ttsdemo.js /frontend/ttsdemo.js
    uni.js /frontend/uni.js
    unidemo.css /frontend/unidemo.css
    unidemo.html /frontend/unidemo.html
    unidemo.js /frontend/unidemo.js
    web_push.js /frontend/web_push.js
    highlight/CHANGES.md /frontend/highlight/CHANGES.md
    highlight/LICENSE /frontend/highlight/LICENSE
    highlight/README.md /frontend/highlight/README.md
    highlight/README.ru.md /frontend/highlight/README.ru.md
    highlight/highlight.pack.js /frontend/highlight/highlight.pack.js
    highlight/styles/agate.css /frontend/highlight/styles/agate.css
    highlight/styles/androidstudio.css /frontend/highlight/styles/androidstudio.css
    highlight/styles/arduino-light.css /frontend/highlight/styles/arduino-light.css
    highlight/styles/arta.css /frontend/highlight/styles/arta.css
    highlight/styles/ascetic.css /frontend/highlight/styles/ascetic.css
    highlight/styles/atelier-cave-dark.css /frontend/highlight/styles/atelier-cave-dark.css
    highlight/styles/atelier-cave-light.css /frontend/highlight/styles/atelier-cave-light.css
    highlight/styles/atelier-dune-dark.css /frontend/highlight/styles/atelier-dune-dark.css
    highlight/styles/atelier-dune-light.css /frontend/highlight/styles/atelier-dune-light.css
    highlight/styles/atelier-estuary-dark.css /frontend/highlight/styles/atelier-estuary-dark.css
    highlight/styles/atelier-estuary-light.css /frontend/highlight/styles/atelier-estuary-light.css
    highlight/styles/atelier-forest-dark.css /frontend/highlight/styles/atelier-forest-dark.css
    highlight/styles/atelier-forest-light.css /frontend/highlight/styles/atelier-forest-light.css
    highlight/styles/atelier-heath-dark.css /frontend/highlight/styles/atelier-heath-dark.css
    highlight/styles/atelier-heath-light.css /frontend/highlight/styles/atelier-heath-light.css
    highlight/styles/atelier-lakeside-dark.css /frontend/highlight/styles/atelier-lakeside-dark.css
    highlight/styles/atelier-lakeside-light.css /frontend/highlight/styles/atelier-lakeside-light.css
    highlight/styles/atelier-plateau-dark.css /frontend/highlight/styles/atelier-plateau-dark.css
    highlight/styles/atelier-plateau-light.css /frontend/highlight/styles/atelier-plateau-light.css
    highlight/styles/atelier-savanna-dark.css /frontend/highlight/styles/atelier-savanna-dark.css
    highlight/styles/atelier-savanna-light.css /frontend/highlight/styles/atelier-savanna-light.css
    highlight/styles/atelier-seaside-dark.css /frontend/highlight/styles/atelier-seaside-dark.css
    highlight/styles/atelier-seaside-light.css /frontend/highlight/styles/atelier-seaside-light.css
    highlight/styles/atelier-sulphurpool-dark.css /frontend/highlight/styles/atelier-sulphurpool-dark.css
    highlight/styles/atelier-sulphurpool-light.css /frontend/highlight/styles/atelier-sulphurpool-light.css
    highlight/styles/atom-one-dark.css /frontend/highlight/styles/atom-one-dark.css
    highlight/styles/atom-one-light.css /frontend/highlight/styles/atom-one-light.css
    highlight/styles/brown-paper.css /frontend/highlight/styles/brown-paper.css
    highlight/styles/brown-papersq.png /frontend/highlight/styles/brown-papersq.png
    highlight/styles/codepen-embed.css /frontend/highlight/styles/codepen-embed.css
    highlight/styles/color-brewer.css /frontend/highlight/styles/color-brewer.css
    highlight/styles/darcula.css /frontend/highlight/styles/darcula.css
    highlight/styles/dark.css /frontend/highlight/styles/dark.css
    highlight/styles/darkula.css /frontend/highlight/styles/darkula.css
    highlight/styles/default.css /frontend/highlight/styles/default.css
    highlight/styles/docco.css /frontend/highlight/styles/docco.css
    highlight/styles/dracula.css /frontend/highlight/styles/dracula.css
    highlight/styles/far.css /frontend/highlight/styles/far.css
    highlight/styles/foundation.css /frontend/highlight/styles/foundation.css
    highlight/styles/github-gist.css /frontend/highlight/styles/github-gist.css
    highlight/styles/github.css /frontend/highlight/styles/github.css
    highlight/styles/googlecode.css /frontend/highlight/styles/googlecode.css
    highlight/styles/grayscale.css /frontend/highlight/styles/grayscale.css
    highlight/styles/gruvbox-dark.css /frontend/highlight/styles/gruvbox-dark.css
    highlight/styles/gruvbox-light.css /frontend/highlight/styles/gruvbox-light.css
    highlight/styles/hopscotch.css /frontend/highlight/styles/hopscotch.css
    highlight/styles/hybrid.css /frontend/highlight/styles/hybrid.css
    highlight/styles/idea.css /frontend/highlight/styles/idea.css
    highlight/styles/ir-black.css /frontend/highlight/styles/ir-black.css
    highlight/styles/kimbie.dark.css /frontend/highlight/styles/kimbie.dark.css
    highlight/styles/kimbie.light.css /frontend/highlight/styles/kimbie.light.css
    highlight/styles/magula.css /frontend/highlight/styles/magula.css
    highlight/styles/mono-blue.css /frontend/highlight/styles/mono-blue.css
    highlight/styles/monokai-sublime.css /frontend/highlight/styles/monokai-sublime.css
    highlight/styles/monokai.css /frontend/highlight/styles/monokai.css
    highlight/styles/obsidian.css /frontend/highlight/styles/obsidian.css
    highlight/styles/ocean.css /frontend/highlight/styles/ocean.css
    highlight/styles/paraiso-dark.css /frontend/highlight/styles/paraiso-dark.css
    highlight/styles/paraiso-light.css /frontend/highlight/styles/paraiso-light.css
    highlight/styles/pojoaque.css /frontend/highlight/styles/pojoaque.css
    highlight/styles/pojoaque.jpg /frontend/highlight/styles/pojoaque.jpg
    highlight/styles/purebasic.css /frontend/highlight/styles/purebasic.css
    highlight/styles/qtcreator_dark.css /frontend/highlight/styles/qtcreator_dark.css
    highlight/styles/qtcreator_light.css /frontend/highlight/styles/qtcreator_light.css
    highlight/styles/railscasts.css /frontend/highlight/styles/railscasts.css
    highlight/styles/rainbow.css /frontend/highlight/styles/rainbow.css
    highlight/styles/school-book.css /frontend/highlight/styles/school-book.css
    highlight/styles/school-book.png /frontend/highlight/styles/school-book.png
    highlight/styles/solarized-dark.css /frontend/highlight/styles/solarized-dark.css
    highlight/styles/solarized-light.css /frontend/highlight/styles/solarized-light.css
    highlight/styles/sunburst.css /frontend/highlight/styles/sunburst.css
    highlight/styles/tomorrow-night-blue.css /frontend/highlight/styles/tomorrow-night-blue.css
    highlight/styles/tomorrow-night-bright.css /frontend/highlight/styles/tomorrow-night-bright.css
    highlight/styles/tomorrow-night-eighties.css /frontend/highlight/styles/tomorrow-night-eighties.css
    highlight/styles/tomorrow-night.css /frontend/highlight/styles/tomorrow-night.css
    highlight/styles/tomorrow.css /frontend/highlight/styles/tomorrow.css
    highlight/styles/vs.css /frontend/highlight/styles/vs.css
    highlight/styles/xcode.css /frontend/highlight/styles/xcode.css
    highlight/styles/xt256.css /frontend/highlight/styles/xt256.css
    highlight/styles/zenburn.css /frontend/highlight/styles/zenburn.css
    webspeechkit/Makefile /frontend/webspeechkit/Makefile
    webspeechkit/README.md /frontend/webspeechkit/README.md
    webspeechkit/recognizer.js /frontend/webspeechkit/recognizer.js
    webspeechkit/recorder.js /frontend/webspeechkit/recorder.js
    webspeechkit/recorderWorker.js /frontend/webspeechkit/recorderWorker.js
    webspeechkit/recorderWorkerCreator.js /frontend/webspeechkit/recorderWorkerCreator.js
    webspeechkit/recorderWorkerSrc.js /frontend/webspeechkit/recorderWorkerSrc.js
    webspeechkit/speechrecognition.js /frontend/webspeechkit/speechrecognition.js
    webspeechkit/tts.js /frontend/webspeechkit/tts.js
    webspeechkit/webaudiowrapper.js /frontend/webspeechkit/webaudiowrapper.js
    webspeechkit/build/webspeechkit-settings.js /frontend/webspeechkit/build/webspeechkit-settings.js
    webspeechkit/docs/tutorials/quickstart.md /frontend/webspeechkit/docs/tutorials/quickstart.md
    webspeechkit/docs/tutorials/tutorial.json /frontend/webspeechkit/docs/tutorials/tutorial.json
    webspeechkit/extras/bitstring.js /frontend/webspeechkit/extras/bitstring.js
    webspeechkit/extras/speex.js /frontend/webspeechkit/extras/speex.js
    webspeechkit/extras/speex.min.js /frontend/webspeechkit/extras/speex.min.js
    webspeechkit/extras/speexConverter.js /frontend/webspeechkit/extras/speexConverter.js
    webspeechkit/extras/vad.js /frontend/webspeechkit/extras/vad.js
    webspeechkit/internal/analyser.js /frontend/webspeechkit/internal/analyser.js
    webspeechkit/internal/speakerid.js /frontend/webspeechkit/internal/speakerid.js
    webspeechkit/ui/equalizer.js /frontend/webspeechkit/ui/equalizer.js
    webspeechkit/ui/textline.js /frontend/webspeechkit/ui/textline.js
)

END()