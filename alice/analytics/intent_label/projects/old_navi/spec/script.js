window.instruction = $intent_tree;  // Значение подставляется из репозитория

Handlebars.registerHelper('list', function (items, options) {
    var out = "<table class='m-table'>";
    if (items.length % 2 == 0) {
        var alisa = true;
    }
    for (var i = 0, l = items.length; i < l - 1; i++) {
        out = out + "<tr style=\"font-size: 14px; color: #404040;\">";
        if (alisa) {
            out += "<td id=\"dialog\"><i class='material-icons'>mic</i><i class='gray'>Помощник:</i></td>";
        }
        else {
            out += "<td id=\"dialog\"><i class='material-icons'>person</i><i class='gray'>Пользователь:</i></td>";
        }
        alisa = !alisa;
        out = out + "<td id=\"dialog\">" + items[i] + "</td><tr/>";
    }
    out = out + "<tr style=\"font-size: 14px;\">";
    if (alisa) {
        out += "<td id=\"dialog\"><i class='material-icons'>mic</i><i><b class='gray'>Помощник:</b></i></td>";
    }
    else {
        out += "<td id=\"dialog\"><i class='material-icons'>person</i><i><b class='gray'>Пользователь:</b></i></td>";
    }
    alisa = !alisa;
    out = out + "<td id=\"dialog\" class=\"last\"><b>" + items[i] + "</b><a href=\"http://yandex.ru/yandsearch?text=" + items[i] + "\" class=\"out-link\" title=\"Искать в Яндексе\" target=\"_blank\"></a></td><tr/>";
    return out + "</table>";
});

function disableButton(button) {
    button.classList.remove('button_type_action');
    button.classList.add('button_type_normal');
}

function enableButton(button) {
    button.classList.remove('button_type_normal');
    button.classList.add('button_type_action');
}

function switchButton(button) {
    button.classList.toggle('button_type_normal');
    button.classList.toggle('button_type_action');
}

function setVisibility(button, visible) {
    if (!visible) {
        disableButton(button);
        button.style.display = "none";
    }
    else {
        button.style.display = "block";
        button.style.opacity = 1;
    }
}

function setOpacityLow(button) {
    button.style.opacity = 0.25;
}

function setText(button, text) {
    button.querySelector("span").innerHTML = text;
}


//Порядок горячих клавиш
const sequenceEng = '1234567890qwertyuiopasdfghjkl';
const sequenceRus = '1234567890йцукенгшщзфывапролд';
var curTask = 0, scrollingNow=false, suite, searchingNow=false;
exports.Assignment = extend(TolokaAssignment, function (options) {
    TolokaAssignment.call(this, options);
}, {
    resume: function () {
        const options = this.getOptions();
        let that = this;
        if (!window.instruction) {
            //Подгрузка instruction
            $.getJSON('https://support-templates.s3.yandex.net/phrases/phrases.json?myphonenumber=+7' + new Date().getTime(), function (data) {
                window.instruction = data;
                that.getTaskSuite().resume();
                that.onResume();
                that.start();
            });
        }
        else {
            this.getTaskSuite().resume();
            this.onResume();
            this.start();
        }
    }
});
exports.Task = extend(TolokaHandlebarsTask, function (options) {
    TolokaHandlebarsTask.call(this, options);
    this.finalAnswer = false;
    this.resultIntent = "";
}, {
    /*getTemplateData: function () {
		let templateData = TolokaHandlebarsTask.prototype.getTemplateData.call(this);


		return templateData;
	},*/
    validate: function (solution) {
        // this.hideAllHints();
        // console.log(this.finalAnswer);
        if (!this.finalAnswer) {
            // this.render();
            //Перематываем к этому таску
            let task = this.getDOMElement();
            let root=this.getDOMElement().parentElement;
            curTask = task.dataset.number;
            setTimeout(function() {
                suite.focusTask(curTask, true);
                $(root).animate({scrollTop: task.offsetTop}, 200);
                setTimeout(function() {root.querySelectorAll('.task .hiddenField')[curTask].focus();
                scrollingNow=false;}, 250);
            }, 50);
            return {
                task_id: this.getTask().id
                , errors: {
                    "__TASK__": {
                        message: "Не до конца уточнена тема"
                    }
                }
            };
        }
        else {
            return TolokaHandlebarsTask.prototype.validate.apply(this, solution);
        }
    }
    , getSolution: function () {
        var solution = TolokaHandlebarsTask.prototype.getSolution.apply(this);
        // console.log(solution);
        solution.output_values["intent"] = this.resultIntent;
        //console.log(this.resultIntent);
        return solution;
    }
    , hideAllHints: function () {
        var wrapper = this.getDOMElement();
        var hints = wrapper.querySelectorAll(".correct-answer");
        for (var i = 0; i < hints.length; i++) {
            hints[i].className = "correct-answer";
        }
    },
    onRender: function () {
        var states = [];
        var buttons = [];
        var finalAnswerLocal = this.finalAnswer;
        var resultIntentLocal = this.resultIntent;
        this.level_id = 1;
        let that = this;
        let root = this.getDOMElement();
        let sInput=root.querySelector('.searchField input');
        let suggestContainer=root.querySelector('.searchField .suggestContainer');
        let hf = this.getDOMElement().querySelector('.hiddenField');

        //Переключатель горячих клавиш
        let hkSwitcher=root.querySelector('.toggleHotkeys');
        hkSwitcher.addEventListener('click', function() {
           if (root.parentElement.classList.contains('hotkeysDisabled')) {
               root.parentElement.classList.remove('hotkeysDisabled');
               that.storage.setItem('funfrog518hotkeysEnabled', 'true', new Date().getTime() + 7776000000);
           } else {
               root.parentElement.classList.add('hotkeysDisabled');
               that.storage.setItem('funfrog518hotkeysEnabled', 'false', new Date().getTime() + 7776000000);
           }
        });

        //Поле для поиска тематики
        function clearSuggestContainer() {
            suggestContainer.innerHTML='';
        }
        function assemblePath(pathArr) {
            let path='';
            pathArr.forEach(function(buttName) {
               path+=buttName+' -> ';
            });
            return path.substr(0, path.length-4);
        }
        function addSuggest(buttObj) {
            let suggest=document.createElement('div');
            suggest.className='suggest';
            if (buttObj.subitems) {suggest.classList.add('notFinal');} else {suggest.classList.add('final');}
            let titleText='<div class="buttPath">'+assemblePath(buttObj.path)+'</div>';
            if (buttObj.instruction) {titleText+='\n'+buttObj.instruction;}
            suggest.innerHTML=buttObj.name+'<div class="popUpTitle">'+titleText+'</div>';
            suggest.position=buttObj.position;
            suggest.addEventListener('click', function() {
                suggestContainer.classList.remove('open');
                //Сначала возвращаемся в начало
                resetSelection();
                //А теперь щёлкаем по нужным кнопкам
                buttObj.position.forEach(function(index) {
                   clickButton(index);
                });
            });
            suggestContainer.appendChild(suggest);
        }
        sInput.addEventListener('focus', function() {
            searchingNow=true;
            if (this.value) {suggestContainer.classList.add('open');}
        });
        sInput.addEventListener('blur', function() {
            setTimeout(function() {root.querySelectorAll('.hiddenField')[curTask].focus();}, 50);
            searchingNow=false;
        });
        function sInputChanged() {
            clearSuggestContainer();
            if (!sInput.value) {
                suggestContainer.classList.remove('open');
                sInput.parentElement.classList.remove('hasSomething');
                return;
            } else {
                suggestContainer.classList.add('open');
                sInput.parentElement.classList.add('hasSomething');
            }
            let query=sInput.value.toLowerCase();
            let thereWasSomething=false;
            window.buttonsArray.forEach(function(buttObj) {
                if (buttObj.name.includes(query)) {
                    addSuggest(buttObj);
                    thereWasSomething=true;
                }
            });
            if (!thereWasSomething) {suggestContainer.classList.remove('open'); return;}
        }
        sInput.addEventListener('input', function() {
            sInputChanged();
        });
        sInput.parentElement.querySelector('i').addEventListener('click', function() {
            sInput.value='';
            sInputChanged();
        });

        function getSeqIndex(key) {
            if (sequenceEng.indexOf(key) >= 0) {
                return sequenceEng.indexOf(key);
            }
            if (sequenceRus.indexOf(key) >= 0) {
                return sequenceRus.indexOf(key);
            }
            return false;
        }
        //Вешаем обработку на скрытое поле для отслеживания нажатых клавиш
        hf.addEventListener('input', function () {
            if (hf.value == ' ') {
                searchingNow=true;
                sInput.focus();
            }
            if (hf.value == '`' || hf.value == 'ё') {
                if (that.buttons[that.level_id - 2] && that.buttons[that.level_id - 2][that.states[that.level_id - 2]]) {
                    getBack();
                }
            }
            if (hf.value == '=' || hf.value == ']' || hf.value == 'ъ' || hf.value == 'v' || hf.value == 'м') {
                resetSelection();
            }
            else
            if (hf.value == 'z' || hf.value == 'я') {
                that.getDOMElement().parentElement.querySelectorAll('.leftScroll')[curTask].click();
            }
            else
            if (hf.value == 'x' || hf.value == 'ч') {
                that.getDOMElement().parentElement.querySelectorAll('.rightScroll')[curTask].click();
            }
            else
                clickButton(getSeqIndex(hf.value));
            hf.value = '';
        })

        function clickButton(index) {
            if (index===false) {return;}
            if (that.buttons[that.level_id - 1] && that.buttons[that.level_id - 1][index] && that.buttons[that.level_id - 1][index].title) {
                that.buttons[that.level_id - 1][index].click();
            }
        }

        function resetSelection() {
            while (that.buttons[that.level_id - 2] && that.buttons[that.level_id - 2][that.states[that.level_id - 2]]) {
                getBack();
            }
        }

        function activateButton(button) {
            var tokens = button.id.split("_");
            // alert(button.id);
            var level_id = parseInt(tokens[1]);
            var button_id = parseInt(tokens[2]);
            var item = window.instruction;
            if (that.level_id > 1 && level_id < that.level_id) {
                that.level_id = level_id;
                finalAnswerLocal = false;
                newGetBack(button);
                return;
            }
            if (level_id - 1 < states.length) {
                return;
            }
            enableButton(button);
            states.push(button_id - 1);
            for (var i = 0; i < states.length; i++) {
                if (!item || !item["subitems"] || item["subitems"] == "null") {
                    item = null;
                    break;
                }
                item = item["subitems"][states[i]];
            }
            if (!item || item === undefined) {
                finalAnswerLocal = false;
                // console.log("unfinal");
                states.pop();
                that.level_id = level_id;
                getBack();
                return;
            }
            /*for (var i = 0; i < buttons[level_id - 1].length; i++) {
              if (i == button_id - 1) { continue; }
              // alert(i);
              // setVisibility(buttons[level_id - 1][i], false);
              setOpacityLow(buttons[level_id - 1][i]);
            }*/
            if (item["subitems"] && item["subitems"] != "null") {
                finalAnswerLocal = false;
                writeButtons(level_id, item);
                /*setVisibility(buttons[level_id][i], true);
                setText(buttons[level_id][i], "< Назад");
                buttons[level_id][i].title = "Вернуться на предыдущий уровень";*/
            }
            else {
                var resultIntentList = [];
                var item = window.instruction;
                for (var i = 0; i < states.length; i++) {
                    if (!item || !item["subitems"] || item["subitems"] == "null") {
                        item = null;
                        break;
                    }
                    item = item["subitems"][states[i]];
                    resultIntentLocal = item["intent"] || "";
                }
                // resultIntentLocal = resultIntentList.join(".");
                // console.log(resultIntentList);
                finalAnswerLocal = true;
                //that.blur();
                // console.log("final");
                /*setVisibility(buttons[level_id][button_id - 1], true);
                setText(buttons[level_id][button_id - 1], "< Назад");
                buttons[level_id][button_id - 1].title = "Вернуться на предыдущий уровень";*/
            }
            that.level_id = level_id + 1;
            that.states = states;
            //Переводим фокус на отслеживающее кнопки поле
            //hf.focus();
        }
        //Записует названия кнопок в тэдэшки таблицы, тут это работает именно так...
        function writeButtons(level_id, item) {
            for (var i = 0; i < item["subitems"].length; i++) {
                setVisibility(buttons[level_id][i], true);
                setText(buttons[level_id][i], '<span class="buttNumber">' + sequenceEng[i] + '</span>' + item["subitems"][i]["name"] + (item["subitems"][i]["subitems"] && item["subitems"][i]["subitems"] != "null" ? '<i class="material-icons">keyboard_arrow_right</i>' : ''/*'<i class="material-icons">done</i>'*/));
                buttons[level_id][i].title = item["subitems"][i]["name"] + "\n" + item["subitems"][i]["instruction"];
                //Добавляем к тупиковым ветвям кнопочной эволюции пищевой краситель E142
                if (item["subitems"][i]["subitems"] && item["subitems"][i]["subitems"] != "null") {
                    buttons[level_id][i].classList.remove('e142');
                }
                else {
                    buttons[level_id][i].classList.add('e142');
                }
            }
        }

        function newGetBack(buttonPressed) {
            var level_id = states.length;
            that.level_id = 0;
            for (var i = 0; i < buttons[level_id].length; i++) {
                setVisibility(buttons[level_id][i], false);
            }
            for (var i = 0; i < buttons[level_id - 1].length; i++) {
                setVisibility(buttons[level_id - 1][i], false);
                disableButton(buttons[level_id - 1][i]);
            }
            states.pop();
            if (states.length > 0) {
                button = buttons[level_id - 2][states[states.length - 1]];
                //console.log(button);
                states.pop();
                activateButton(button);
                activateButton(buttonPressed);
            }
            else {
                states.pop();
                writeButtons(0, window.instruction);
                activateButton(buttonPressed);
            }
        }

        function getBack() {
            that.level_id = 1;
            var level_id = states.length;
            for (var i = 0; i < buttons[level_id].length; i++) {
                setVisibility(buttons[level_id][i], false);
            }
            for (var i = 0; i < buttons[level_id - 1].length; i++) {
                setVisibility(buttons[level_id - 1][i], false);
                disableButton(buttons[level_id - 1][i]);
            }
            states.pop();
            if (states.length > 0) {
                button = buttons[level_id - 2][states[states.length - 1]];
                states.pop();
                activateButton(button);
            }
            else {
                states.pop();
                writeButtons(0, window.instruction);
            }
        }
        for (var i = 1; i <= 4; i++) {
            buttons.push([]);
            for (var j = 1; j <= 15; j++) {
                var button = this.getDOMElement().querySelector("#button_" + i.toString() + "_" + j.toString());
                button.addEventListener("click", function () {
                    activateButton(this);
                    that.finalAnswer = finalAnswerLocal;
                    that.resultIntent = resultIntentLocal;
                    // console.log(that.finalAnswer);
                    // console.log(that.resultIntent);
                });
                buttons[i - 1].push(button);
            }
        }
        this.buttons = buttons;
        writeButtons(0, window.instruction);
        /*for (int i = 0; i < window.instruction.length(); i++) {
         buttons[0]
        }*/
        // button1.classList.remove('button_type_action');
        // button1.classList.add('button_type_normal');
        // button1.style.visibility = "hidden";
    }
    , onDestroy: function () {
        // Задание завершено, можно освобождать (если были использованы) глобальные ресурсы
    }
});
//Тут только смена тем и листалка
exports.TaskSuite = extend(TolokaHandlebarsTaskSuite, function (options) {
    TolokaHandlebarsTaskSuite.call(this, options);
 }, {
    onRender() {
        let root = this.getDOMElement();
        let _this = this;
        suite=this;

        //Вспомогательный массив кнопок для более рациональной работы и более простой реализации поиска с саджестом
        window.buttonsArray=[];
        function subItems2Array(obj, pos, path) { //pos и path - это одно и то же, только pos в индексах, а path - в названиях
            //Если у объекта есть имя - генерируем объект из имени и позиции и заносим в массив для дальнейшего доступа
            if (obj.name && obj.name.length>1) {
                let buttObj={};
                buttObj.name=obj.name.toLowerCase();
                buttObj.position=pos.slice(0);
                buttObj.instruction=obj.instruction;
                if (obj.subitems) {buttObj.subitems=true;} else {buttObj.subitems=false;}
                buttObj.path=path.slice(0)
                window.buttonsArray.push(buttObj)
            }
            //Если у объекта есть дочерние - вызываем для них то же самое рекурсивно
            if (obj.subitems) {
                for (let i=0; i<obj.subitems.length; i++) {
                    let newArr=pos.slice(0);
                    newArr.push(i);
                    let newArr2=path.slice(0);
                    newArr2.push(obj.subitems[i].name);
                    subItems2Array(obj.subitems[i], newArr, newArr2);
                }
            }
        }
        subItems2Array(window.instruction, [], []);


        //Проставляем номера и количество тасков в листалках и биндим действия на кнопки
        root.style.left = 0;
        let tasks = root.querySelectorAll('.task');
        for (let i = 0; i < tasks.length; i++) {
            tasks[i].querySelector('.curTab').innerHTML = i + 1;
            tasks[i].querySelector('.tabsCount').innerHTML = tasks.length;
            tasks[i].dataset.number = i;
            tasks[i].querySelector('.leftScroll').addEventListener('click', function () {
                if (i > 0) {
                    scrollingNow=true;
                    curTask = i - 1;
                    setTimeout(function() {
                        _this.focusTask(curTask, true);
                        $(root).animate({scrollTop: tasks[curTask].offsetTop}, 200);
                        setTimeout(function() {
                            root.querySelectorAll('.task .hiddenField')[curTask].focus();
                            scrollingNow=false;}, 250);
                    }, 50);
                }
            });
            tasks[i].querySelector('.rightScroll').addEventListener('click', function () {
                if (i < tasks.length - 1) {
                    scrollingNow=true;
                    curTask = i + 1;
                    setTimeout(function() {
                        _this.focusTask(curTask, true);
                        $(root).animate({scrollTop: tasks[curTask].offsetTop}, 200);
                        setTimeout(function() {root.querySelectorAll('.task .hiddenField')[curTask].focus();
                        scrollingNow=false;}, 250);
                    }, 50);
               }
            });
        }
        //Запускаем постоянное перекидывание фокуса на скрытый инпут текущего таска для отслеживания нажатых кнопок
        setInterval(function () {
            if (!scrollingNow && !searchingNow) {
               	if (!root.querySelector('.task_focused') || root.classList.contains('hotkeysDisabled')) {
                  return;
                }
                curTask=root.querySelector('.task_focused').dataset.number;
                let y = root.scrollTop;
                root.querySelectorAll('.task .hiddenField')[curTask].focus();
                root.scrollTo(0, y);
            }
        }, 500);

        //Восстанавливаем из кук состояние переключателя горячих клавиш
        if (!_this.storage.getItem('funfrog518hotkeysEnabled') === true) {
            root.classList.add('hotkeysDisabled');
        }


        //Включаем светлую тему для джедаев
        let switchButton = root.querySelector('.task:last-child .btnDarkSide a');
        if (_this.storage.getItem('funfrogLightSide') === false) {
            switchButton.innerHTML = '<i class="material-icons">invert_colors</i>Перейти на светлую сторону';
            root.classList.add('darkSide');
        }
        //Going full Anakin and back
        switchButton.addEventListener("click", function () {
            if (root.classList.contains('darkSide')) {
                this.innerHTML = '<i class="material-icons">invert_colors</i>Перейти на тёмную сторону';
                root.classList.remove('darkSide');
                _this.storage.setItem('funfrogLightSide', 'true', new Date().getTime() + 7776000000);
            }
            else {
                this.innerHTML = '<i class="material-icons">invert_colors</i>Перейти на светлую сторону';
                root.classList.add('darkSide');
                _this.storage.setItem('funfrogLightSide', 'false', new Date().getTime() + 7776000000);
            }
        });
    }
});

function extend(ParentClass, constructorFunction, prototypeHash) {
    constructorFunction = constructorFunction || function () {};
    prototypeHash = prototypeHash || {};
    if (ParentClass) {
        constructorFunction.prototype = Object.create(ParentClass.prototype);
    }
    for (var i in prototypeHash) {
        constructorFunction.prototype[i] = prototypeHash[i];
    }
    return constructorFunction;
}