<?php

$prev_com = "";
$prev_cur = "";
$test = "";
$user = "";
$current = "";
$include = "";
$scene = "";
$name = "";
$wish1 = "";
$wish2 = "";
$prev_scene = "";


function out($text, $opt = array(), $tts = false, $end_session = false, $activationtype = null){
    global $current, $prev_com, $prev_cur, $test, $user, $current, $include, $scene, $name, $wish1, $wish2, $prev_scene;
	if(!$tts) $tts = str_replace("\n", ' - ', $text);
    if(isset($opt["img"])) $opt["img"] = ["type" => "BigImage", "image_id" => $opt["img"], "description" => $text];
    if(isset($opt["fimg"])) $opt["img"] = $opt["fimg"];
	
	if ($activationtype == null) {
        $r = json_encode([
            'version' => '1.0',
            'response' => [
                'text' => $text,
                'tts' => $tts,
                'card' => $opt["img"],
                'buttons' => $opt["buttons"],
                'end_session' => $end_session
            ],
            'session_state' => [
                'prev_com' => $prev_com,
                'prev_cur' => $prev_cur,
                'current' =>  $current,
                'test' => $test,
            ],
            'user_state_update' => [
				'value' => $user,
				'include' => $include,
				'scene' => $scene,
				'name' => $name,
				'prev_scene' => $prev_scene,
				'wish1' => $wish1,
				'wish2' => $wish2
            ]
        ]);
    } else {
        $r = json_encode([
            'version' => '1.0',
            'response' => [
                'text' => $text,
                'tts' => $tts,
                'card' => $opt["img"],
                'buttons' => $opt["buttons"],
                'end_session' => $end_session,
                'directives' => [
                    'activate_skill_product' => [
                        'activation_type' => $activationtype
                    ]
                ]
            ],
            'session_state' => [
                'prev_com' => $prev_com,
                'prev_cur' => $prev_cur,
                'current' =>  $current,
                'test' => $test,
            ],
            'user_state_update' => [
				'value' => $user,
				'include' => $include,
				'scene' => $scene,
				'name' => $name,
				'prev_scene' => $prev_scene,
				'wish1' => $wish1,
				'wish2' => $wish2
            ]
        ]);
	}
	print_r($r);
    return $r;
}


function handler($event, $context){
	
	print_r(json_encode($event));
	if (!isset($event['session'])) {
		return "";
	}else{

		global $prev_com, $prev_cur, $test, $user, $current, $include, $scene, $name, $wish1, $wish2, $prev_scene;
		$prev_com = $event['state']['session']['prev_com'];
        $prev_cur = $event['state']['session']['prev_cur'];
        $current = $event['state']['session']['current'];
        $test = $event['state']['session']['test'];
        
        $user = $event['state']['user']['value'];
        $include = $event['state']['user']['include'];
		$scene = $event['state']['user']['scene'];
		$name = $event['state']['user']['name'];
		$prev_scene = $event['state']['user']['prev_scene'];
		$wish1 = $event['state']['user']['wish1'];
		$wish2 = $event['state']['user']['wish2'];


		$u = $event['session']['user_id'];
		$command = mb_strtolower($event['request']['command'].$event['request']['payload'], 'UTF-8');
	}



	$yes = $event['request']['nlu']['intents']['Yes'];
    $no = $event['request']['nlu']['intents']['No'];
    $returning = $event['request']['nlu']['intents']['return_to_beginning'];
	
	if ($yes) {
		$command = 'да';
	}
	if ($no) {
		$command = 'нет';
    }
	if ($returning) {
		$command = 'сначала';
	}
	if (strpos($command, 'мох') !== false || strpos($command, 'мхом') !== false){
		$command = 'гриб';
	}


	if(strpos($current, 'activation1') !== false){
        if(strpos($command, 'да') !== false){
            $text = 'Тогда мне нужно с ней познакомиться. С кем ты хочешь меня познакомить?';     
		    $tts = 'Тогда мне нужно с ней познакомиться. <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/88edfb35-26a1-4f9b-a1d3-f44f83cd21ac.opus">';
		    $current = 'activation2';
            return out($text, ["buttons" => [["title" => "Анна", "hide" => true, "payload" => "анна"],["title" => "Эльза", "hide" => true, "payload" => "эльза"],["title" => "Олаф", "hide" => true, "payload" => "олаф"],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
        } else {
            $text = 'Без фигурки я не могу отправиться в путешествие. Возвращайся с Анной, Эльзой или Олафом — и я сразу перенесу тебя в Эренделл.';     
		    $tts = 'Без фигурки я не могу отправиться в путешествие. Возвращайся с Анной, +Эльзой или Олафом — и я сразу перенесу тебя в Эренделл.';
		    $current = 'activation2';
            return out($text, ["buttons" => [["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts, true);
        }	
	}

	if(strpos($current, 'activation2') !== false || strpos($command, 'познакомься') !== false || strpos($command, 'активир') !== false){
        if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
            $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
            $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
            return out($text, [], $tts, true);
        }
        $el = false;
		$an = false;
		$ol = false;
		foreach($event['session']['user']['skill_products'] as $sp){
            if (strpos($sp['uuid'], '321e4f75-3234-4f8b-94c2-4cf2bd9cd147') !== false) {
				$el = true;
            }
            if (strpos($sp['uuid'], '38b7d4b6-6c27-456f-be10-ae1d9e23ae6b') !== false) {
                $an = true;
            }
            if (strpos($sp['uuid'], '6844b936-e413-4cef-9e1e-3a3a08649cd4') !== false) {
                $ol = true;
            }
		}

		if (strpos($command, 'эльз') !== false) {
			if ($el === true) {
				$include = 'elza.php';
			} else {
				$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
				$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else if (strpos($command, 'ан') !== false) {
			if ($an === true) {
				$include = 'anna.php';
			} else {
				$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
				$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else if (strpos($command, 'ол') !== false) {
			if ($ol === true) {
				$include = 'olaf.php';
			} else {
				$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
				$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else {
			$text = 'С кем ты хочешь меня познакомить?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/88edfb35-26a1-4f9b-a1d3-f44f83cd21ac.opus">';
			$current = 'activation2';
			return out($text, ["buttons" => [["title" => "Анна", "hide" => true, "payload" => "анна"],["title" => "Эльза", "hide" => true, "payload" => "эльза"],["title" => "Олаф", "hide" => true, "payload" => "олаф"],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);	
		}


		if ($name === ""){
			$scene = 0;
			$prev_scene = 0;
		} else {
			$scene = 8;
			$prev_scene = 8;
		}

		goto scene;
	}




	if($event['session']['new'] == true){
		$wish1 = "";
		$wish2 = "";
		if(strpos($command, 'эльз') !== false || strpos($command, 'ан') !== false || strpos($command, 'ол') !== false){
            if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                return out($text, [], $tts, true);
            }
            $el = false;
			$an = false;
			$ol = false;
			foreach($event['session']['user']['skill_products'] as $sp){
				if (strpos($sp['uuid'], '321e4f75-3234-4f8b-94c2-4cf2bd9cd147') !== false) {
					$el = true;
				}
				if (strpos($sp['uuid'], '38b7d4b6-6c27-456f-be10-ae1d9e23ae6b') !== false) {
					$an = true;
				}
				if (strpos($sp['uuid'], '6844b936-e413-4cef-9e1e-3a3a08649cd4') !== false) {
					$ol = true;
				}
			}
	
			if (strpos($command, 'эльз') !== false) {
				if ($el === true) {
					$include = 'elza.php';
				} else {
					$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
					$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
					$current = 'activation3';
					return out($text, [], $tts, false, "music");
				}
			} else if (strpos($command, 'ан') !== false) {
				if ($an === true) {
					$include = 'anna.php';
				} else {
					$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
					$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
					$current = 'activation3';
					return out($text, [], $tts, false, "music");
				}
			} else if (strpos($command, 'ол') !== false) {
				if ($ol === true) {
					$include = 'olaf.php';
				} else {
					$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
					$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
					$current = 'activation3';
					return out($text, [], $tts, false, "music");
				}
			} else {
				$text = 'С кем ты хочешь меня познакомить?';     
				$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/88edfb35-26a1-4f9b-a1d3-f44f83cd21ac.opus">';
				$current = 'activation2';
				return out($text, ["buttons" => [["title" => "Анна", "hide" => true, "payload" => "анна"],["title" => "Эльза", "hide" => true, "payload" => "эльза"],["title" => "Олаф", "hide" => true, "payload" => "олаф"],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);	
			}
	
	
			if ($name === ""){
				$scene = 0;
				$prev_scene = 0;
			} else {
				$scene = 8;
				$prev_scene = 8;
			}
	
			goto scene;
		}

        $text = 'Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".';
        $tts = '<speaker audio="dialogs-upload/f174caa8-6845-41b4-bc8d-53618fd86304/f9102a5d-b910-4743-84f7-d8fdb665fdc8.opus"> Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".';
        $current = 'razvil1';
        return out($text, ['fimg' => [
            "type" => "BigImage",
            "image_id" => '213044/33a5187a0c994c836ff6',
            "description" => $text
        ], "buttons" => [["title" => "Герой", "hide" => true, "payload" => "герой"],["title" => "Путешествие", "hide" => true, "payload" => "путешествие"]]], $tts);
	}


    if(strpos($command, 'начал') !== false){
		$wish1 = "";
		$wish2 = "";
		$text = 'Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".';
        $tts = 'Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".';
        $current = 'razvil1';
        return out($text, ['fimg' => [
            "type" => "BigImage",
            "image_id" => '213044/33a5187a0c994c836ff6',
            "description" => $text
        ], "buttons" => [["title" => "Герой", "hide" => true, "payload" => "герой"],["title" => "Путешествие", "hide" => true, "payload" => "путешествие"]]], $tts);
    }

	
	if (strpos($command, 'помоги') !== false || strpos($command, 'помощь') !== false || strpos($command, 'что ты умеешь') !== false) {
		$text = 'Если тебе плохо слышно или ты не помнишь, что тебе сказали, скажи «Алиса, повтори».
		
Если не можешь решить загадку, скажи «Алиса, подскажи».
		
Если хочешь начать сначала, скажи «Алиса, сначала».
		
Если не понимаешь, как играть, скажи «Алиса, что это?»
		
Если не хочешь больше играть, скажи «Хватит».
		
Чтобы снова позвать на помощь, скажи «Алиса, помоги».
		
Если хочешь пройти игру за Эльзу, скажи «Алиса, хочу помочь Эльзе».
		
Если хочешь пройти игру за Олафа, скажи «Алиса, хочу помочь Олафу».
		
Если хочешь пройти игру за Анну, скажи «Алиса, хочу помочь Анне».';
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/ded7fb87-83e8-4d19-858c-943c569d203a.opus">';
		$current = 'help'.$current;	
        return out($text, ["buttons" => [["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
	}

	if (strpos($command, 'что это') !== false) {
		$text = 'Это игра «Холодное сердце». Внимательно слушай, что говорю я, Анна, Эльза или Олаф, и отвечай. А если что-то не получается, скажи «Алиса, помоги». Ты можешь сыграть за Олафа, Эльзу или Анну, если у тебя есть нужная фигурка.';
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/42b89708-cfb4-47eb-818a-56c24eaf1550.opus">';
		return out($text, ["buttons" => [["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
	}

	if (strpos($command, 'выход') !== false || strpos($command, 'выйти') !== false || strpos($command, 'хватит') !== false) {
		$text = 'Эльза, Олаф и Анна будут ждать твоего возвращения.';
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/c2dc8bcc-1998-4755-94da-3fe86f85e5b7.opus">';
		return out($text, [], $tts, true);
	}


	if (strpos($command, 'хочу помочь') !== false || strpos($current, 'hochupomoch') !== false) {
        if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
            $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
            $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
            return out($text, [], $tts, true);
        }
		$el = false;
		$an = false;
		$ol = false;
		foreach($event['session']['user']['skill_products'] as $sp){
            if (strpos($sp['uuid'], '321e4f75-3234-4f8b-94c2-4cf2bd9cd147') !== false) {
				$el = true;
            }
            if (strpos($sp['uuid'], '38b7d4b6-6c27-456f-be10-ae1d9e23ae6b') !== false) {
                $an = true;
            }
            if (strpos($sp['uuid'], '6844b936-e413-4cef-9e1e-3a3a08649cd4') !== false) {
                $ol = true;
            }
		}

		if (strpos($command, 'эльз') !== false) {
			if ($el === true) {
				$include = 'elza.php';
			} else {
				$text = 'Хорошо. Тогда поднеси фигурку ко мне поближе и скажи «Алиса, познакомься с Эльзой». А потом нажми кнопку на фигурке.';     
				$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/f80bb32c-712b-4314-b50b-21684ba7b51d.opus">';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else if (strpos($command, 'ан') !== false) {
			if ($an === true) {
				$include = 'anna.php';
			} else {
				$text = 'Хорошо. Тогда поднеси фигурку ко мне поближе и скажи «Алиса, познакомься с Анной». А потом нажми кнопку на фигурке.';     
				$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/240f52fb-1d0b-4472-b240-dc499f44b6fc.opus">';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else if (strpos($command, 'ол') !== false) {
			if ($ol === true) {
				$include = 'olaf.php';
			} else {
				$text = 'Хорошо. Тогда поднеси фигурку ко мне поближе и скажи «Алиса, познакомься с Олафом». А потом нажми кнопку на фигурке.';     
				$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/d5f229e0-cbb0-4ceb-8dd4-6d76a29b412e.opus">';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else {
			$text = 'Кому ты хочешь помочь?';
			$tts = 'Кому ты хочешь помочь?';
			$current = 'hochupomoch';
			return out($text, [], $tts);
		}


		if ($name === ""){
			$scene = 0;
			$prev_scene = 0;
		} else {
			$scene = 8;
			$prev_scene = 8;
		}

		goto scene;
	}


	if($include === "elza.php" && ($scene === 9 || $scene === 18)){
		$wish1 = $command;
	}
	if($include === "elza.php" && ($scene === 12 || $scene === 13 || $scene === 14 || $scene === 15 || $scene === 17 || $scene === 25)){
		$wish2 = $command;
	}
 


	if($scene === 5 || $scene === 7){
		$name = mb_strtoupper(mb_substr($command, 0,1)).mb_substr($command, 1);
	}


    if(strpos($current, 'razvil1') !== false && (strpos($command, 'повтор') !== false || strpos($current, 'help') !== false)){
        $text = 'Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".';
        $tts = 'Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".';
        $current = 'razvil1';
        return out($text, ["buttons" => [["title" => "Герой", "hide" => true, "payload" => 'герой'],["title" => "Путешествие", "hide" => true, "payload" => "путешествие"]]], $tts);
    }

	$prodol = false;
    
	if(strpos($command, 'повтор') !== false || strpos($current, 'help') !== false){
		$prodol = true;
        $current = $prev_cur;
		$command = $prev_com;
		$scene = $prev_scene;
	}else{
		$prev_cur = $current;
		$prev_com = $command;
		$prev_scene = $scene;
	}	 


	if(strpos($current, 'razvil1') !== false){
        if(strpos($command, 'путешес') !== false){
            $current = 'razvil2';
            $text = 'С кем ты хочешь сыграть? С Анной, Эльзой или Олафом?';
            $tts = 'С кем ты хочешь сыграть? С Анной, +Эльзой или Олафом?';
            return out($text, ["buttons" => [["title" => "Анна", "hide" => true, "payload" => "анна"],["title" => "Эльза", "hide" => true, "payload" => "эльза"],["title" => "Олаф", "hide" => true, "payload" => "олаф"]]], $tts);
        } else if (strpos($command, 'геро') !== false) {
            $text = 'Отлично. Тебе нужно будет ответить на пять вопросов. А если сомневаешься, скажи «Повтори вопрос». Договорились?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/1554c5e1-d399-4dc7-85d8-48f5b180cdd2.opus">';
			$current = 'start2';
			return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
        } else {
            $text = 'Нет уж, выбирай: герой или путешествие?';     
			$tts = 'Нет уж, выбирай: герой или путешествие?';
			$current = 'razvil1';
			return out($text, ["buttons" => [["title" => "Герой", "hide" => true, "payload" => "герой"],["title" => "Путешествие", "hide" => true, "payload" => "путешествие"]]], $tts);
        }
    }

    if(strpos($current, 'razvil2') !== false){
        if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
            $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
            $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
            return out($text, [], $tts, true);
        }
        $el = false;
		$an = false;
		$ol = false;
		foreach($event['session']['user']['skill_products'] as $sp){
            if (strpos($sp['uuid'], '321e4f75-3234-4f8b-94c2-4cf2bd9cd147') !== false) {
				$el = true;
            }
            if (strpos($sp['uuid'], '38b7d4b6-6c27-456f-be10-ae1d9e23ae6b') !== false) {
                $an = true;
            }
            if (strpos($sp['uuid'], '6844b936-e413-4cef-9e1e-3a3a08649cd4') !== false) {
                $ol = true;
            }
		}

		if (strpos($command, 'эльз') !== false) {
			if ($el === true) {
				$include = 'elza.php';
			} else {
				$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
				$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else if (strpos($command, 'ан') !== false) {
			if ($an === true) {
				$include = 'anna.php';
			} else {
				$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
				$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else if (strpos($command, 'ол') !== false) {
			if ($ol === true) {
				$include = 'olaf.php';
			} else {
				$text = 'Хорошо. Тогда нажми кнопку на фигурке.';     
				$tts = 'Хорошо. Тогда нажми кнопку на фигурке.';
				$current = 'activation3';
				return out($text, [], $tts, false, "music");
			}
		} else {
			$text = 'С кем ты хочешь меня познакомить?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/88edfb35-26a1-4f9b-a1d3-f44f83cd21ac.opus">';
			$current = 'activation2';
			return out($text, ["buttons" => [["title" => "Анна", "hide" => true, "payload" => "анна"],["title" => "Эльза", "hide" => true, "payload" => "эльза"],["title" => "Олаф", "hide" => true, "payload" => "олаф"],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);	
		}


		if ($name === ""){
			$scene = 0;
			$prev_scene = 0;
		} else {
			$scene = 8;
			$prev_scene = 8;
		}

		goto scene;
	}
	

	if(strpos($current, 'start1') !== false){
		if(strpos($command, 'да') !== false || strpos($command, 'привет') !== false){
			$text = 'Отлично. Тебе нужно будет ответить на пять вопросов. А если сомневаешься, скажи «Повтори вопрос». Договорились?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/1554c5e1-d399-4dc7-85d8-48f5b180cdd2.opus">';
			$current = 'start2';
			return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		} else {
			$text = 'Возвращайся, когда захочешь продолжить игру';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/ad369508-c0ec-4b59-b0aa-294bd4540b74.opus">';
			return out($text, [], $tts, true);
		}
	}

	if(strpos($current, 'start2') !== false){
		if(strpos($command, 'да') !== false){
			$text = 'Вот и хорошо. Представь, что ты отправляешься в путешествие и можешь взять с собой одного спутника. Кто это будет: северный олень, снежный тролль или огромная морковка?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/4936702d-4157-464a-963b-f5b04bff22d3.opus">';
			$current = 'test1';
			return out($text, ["buttons" => [["title" => "Олень", "hide" => true, "payload" => 'олень'],["title" => "Тролль", "hide" => true, "payload" => 'тролль'],["title" => "Морковка", "hide" => true, "payload" => 'морковка'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}
		if(strpos($command, 'нет') !== false){
			$text = 'Хочешь закончить игру?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/5dc64de4-1b53-460d-8873-e3ca0ed2058a.opus">';
			$current = 'konec';
			return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'],["title" => "Нет", "hide" => true, "payload" => 'нет'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}
	}

	if(strpos($current, 'konec') !== false && strpos($command, 'нет') !== false){
		$text = 'Отлично. Тебе нужно будет ответить на пять вопросов. А если сомневаешься, скажи «Повтори вопрос». Договорились?';     
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/1554c5e1-d399-4dc7-85d8-48f5b180cdd2.opus">';
		$current = 'start2';
		return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
	}

	
	if(strpos($current, 'konec') !== false && strpos($command, 'да') !== false){
		$text = 'Возвращайся, когда захочешь продолжить игру.';     
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/ad369508-c0ec-4b59-b0aa-294bd4540b74.opus">';
		$current = 'start2';
		return out($text, [], $tts, true);
	}

	if(strpos($current, 'test1') !== false){
		if(strpos($command, 'олен') !== false || strpos($command, 'север') !== false){
			$anna = '1';
			$elza = '0';
			$olaf = '0';
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'трол') !== false || strpos($command, 'снеж') !== false){
			$anna = '0';
			$elza = '1';
			$olaf = '0';
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'морков') !== false || strpos($command, 'огром') !== false){
			$anna = '0';
			$elza = '0';
			$olaf = '1';
			$test = $anna.$elza.$olaf;
		} else {
			$text = 'И всё же: олень, тролль или морковка?';
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/022367ce-c38f-4e0b-ab5d-fe13eb074812.opus">';
			$current = 'test1';
			return out($text, ["buttons" => [["title" => "Олень", "hide" => true, "payload" => 'олень'],["title" => "Тролль", "hide" => true, "payload" => 'тролль'],["title" => "Морковка", "hide" => true, "payload" => 'морковка'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}

		$text = 'Прекрасно. Путешествие началось. Перед тобой широкая пропасть. Как будешь через неё перебираться? Выстроишь мост, спустишься по верёвке или раскатишься с ледяной горки?';     
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/2cfec409-a69f-48fb-b133-74d7137200bc.opus">';
		$current = 'test2';
		return out($text, ["buttons" => [["title" => "Мост", "hide" => true, "payload" => 'мост'],["title" => "Верёвка", "hide" => true, "payload" => 'веревка'],["title" => "Горка", "hide" => true, "payload" => 'горка'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
	}

	if(strpos($current, 'test2') !== false){
		if(strpos($command, 'веревк') !== false || strpos($command, 'спу') !== false){
			$anna = (string)(int)$test[0] + 1;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'мост') !== false || strpos($command, 'выстро') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 1;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'горк') !== false || strpos($command, 'ска') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 1;
			$test = $anna.$elza.$olaf;
		} else {
			$text = 'Подумай. Мост, верёвка или горка?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/ba89505a-147a-4258-a839-1604c6776b4d.opus">';
			$current = 'test2';
			return out($text, ["buttons" => [["title" => "Мост", "hide" => true, "payload" => 'мост'],["title" => "Верёвка", "hide" => true, "payload" => 'веревка'],["title" => "Горка", "hide" => true, "payload" => 'горка'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}

		$text = 'А теперь поднялась снежная вьюга. Что будешь делать? Всё равно пойдёшь вперед? Позовёшь друзей? Или успокоишь вьюгу магией?';     
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/24ad5f71-35cb-45c8-aaa8-c1eaa67bb113.opus">';
		$current = 'test3';
		return out($text, ["buttons" => [["title" => "Вперед", "hide" => true, "payload" => 'вперед'],["title" => "Друзья", "hide" => true, "payload" => 'друзья'],["title" => "Магия", "hide" => true, "payload" => 'магия'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
	}
	
	if(strpos($current, 'test3') !== false){
		if(strpos($command, 'вперед') !== false || strpos($command, 'да') !== false || strpos($command, 'все равно') !== false || strpos($command, 'пойд') !== false){
			$anna = (string)(int)$test[0] + 1;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'маги') !== false || strpos($command, 'успок') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 1;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'друз') !== false || strpos($command, 'позов') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 1;
			$test = $anna.$elza.$olaf;
		} else {
			$text = 'Нет уж. Выбирай: вперёд, друзья или магия?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/55839d72-def0-4f57-9f2a-164de44c864c.opus">';
			$current = 'test3';
			return out($text, ["buttons" => [["title" => "Вперед", "hide" => true, "payload" => 'вперед'],["title" => "Друзья", "hide" => true, "payload" => 'друзья'],["title" => "Магия", "hide" => true, "payload" => 'магия'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}

		$text = 'Теперь ты на перекрёстке трёх дорог. На камне написано: «Направо — лето. Налево — зима. Прямо — обычный год, всё по очереди». Куда пойдёшь.';     
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/61d1de48-64b3-4ca6-b580-181ad1adc2c9.opus">';
		$current = 'test4';
		return out($text, ["buttons" => [["title" => "Налево", "hide" => true, "payload" => 'налево'],["title" => "Прямо", "hide" => true, "payload" => 'прямо'],["title" => "Направо", "hide" => true, "payload" => 'направо'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
	}

	if(strpos($current, 'test4') !== false){
		if(strpos($command, 'прямо') !== false || strpos($command, 'обычный') !== false){
			$anna = (string)(int)$test[0] + 1;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'лево') !== false || strpos($command, 'зим') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 1;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'право') !== false || strpos($command, 'лето') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 1;
			$test = $anna.$elza.$olaf;
		} else {
			$text = 'Увы, дороги всего три. Лето, зима или обычный год?';
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/ca0254a2-91f7-4f61-be9e-69cf12f2f8c5.opus">';
			$current = 'test4';
			return out($text, ["buttons" => [["title" => "Лето", "hide" => true, "payload" => 'лето'],["title" => "Зима", "hide" => true, "payload" => 'зима'],["title" => "Обычный год", "hide" => true, "payload" => 'обычный год'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}

		$text = 'И последний вопрос. В конце путешествия тебя ждёт волшебное зеркало. Взгляни в него: какого цвета твои волосы? Рыжевато-каштановые? Белые как снег? А может, их вовсе нет? ';     
		$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/7f1ee979-090c-41c9-af15-3accf8aac103.opus">';
		$current = 'test5';
		return out($text, ["buttons" => [["title" => "Рыжевато-каштановые", "hide" => true, "payload" => 'Рыжевато-каштановые'],["title" => "Белые", "hide" => true, "payload" => 'белые'],["title" => "Их вовсе нет", "hide" => true, "payload" => 'их вовсе нет'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
	}

	if(strpos($current, 'test5') !== false){
		if(strpos($command, 'каштан') !== false || strpos($command, 'рыж') !== false || strpos($command, 'рыжевато-каштановые') !== false || strpos($command, 'коричневые') !== false || strpos($command, 'темн') !== false){
			$anna = (string)(int)$test[0] + 1;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'белые') !== false || strpos($command, 'светлые') !== false || strpos($command, 'сне') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 1;
			$olaf = (string)(int)$test[2] + 0;
			$test = $anna.$elza.$olaf;
		} else if(strpos($command, 'нет') !== false || strpos($command, 'лыс') !== false){
			$anna = (string)(int)$test[0] + 0;
			$elza = (string)(int)$test[1] + 0;
			$olaf = (string)(int)$test[2] + 1;
			$test = $anna.$elza.$olaf;
		} else {
			$text = 'Посмотри повнимательнее. Каштановые, белые или вовсе нет?';
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/91087b1e-5826-4f37-b9da-8cb71d1fd2f7.opus">';
			$current = 'test5';
			return out($text, ["buttons" => [["title" => "Каштановые", "hide" => true, "payload" => 'Каштановые'],["title" => "Белые", "hide" => true, "payload" => 'белые'],["title" => "Вовсе нет", "hide" => true, "payload" => 'вовсе нет'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}

		if($anna >= $elza && $anna >= $olaf){
			$text = 'Что ж, слушай. В «Холодном сердце» ты — Анна. Добрая, смелая и неугомонная принцесса Эренделла, которая обожает приключения и всегда выпутывается из любых трудностей.';
            $tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/4c1edd63-2fdb-4d02-b4c5-054817790ed9.opus">';
            $image = '1540737/5e17c562bf94e00421d4';
            $link = 'https://yandex.ru/alice/frozen/anna';
		} else if($elza >= $anna && $elza >= $olaf){
			$text = 'Что ж, слушай. В «Холодном сердце» ты — Эльза. Благородная королева Эренделла, которая владеет снежной магией и обожает свою младшую сестру Анну.';
            $tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/a173ff20-606c-4cbb-894e-a69b878d697f.opus">';
            $image = '1652229/03662b831e2f8e61511b';
            $link = 'https://yandex.ru/alice/frozen/elza';
		} else {
			$text = 'Что ж, слушай. В «Холодном сердце» ты — Олаф. Добрый и отважный волшебный снеговик с морковкой вместо носа. Олаф никогда не унывает, обожает друзей, лето и жаркие объятия.';
            $tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/dc3e2888-77c9-41d4-93df-66554f7b4afd.opus">';
            $image = '213044/19b356541e462b404c6b';
            $link = 'https://yandex.ru/alice/frozen/olaf';
		}

		$text = $text.' А теперь хочешь отправиться в волшебное путешествие вместе с Эльзой, Анной и Олафом?';     

		$tts = $tts.'<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/2dccf1d7-d776-411c-b3e4-94a2666d55f8.opus">';
		$current = 'test6';
        return out($text, ['fimg' => [
            "type" => "BigImage",
            "image_id" => $image,
            "description" => $text, 
            "button" => ["url" => $link]
        ], "buttons" => [["title" => "Да", "hide" => true, "payload" => "да"], ["title" => 'Поделиться', "hide" => true, "url" => $link], ["title" => "С начала", "hide" => true, "payload" => "сначала"]]], $tts);
    }

	if(strpos($current, 'test6') !== false){
		if(strpos($command, 'да') !== false){
			$text = 'А у тебя есть волшебная фигурка?';     
			$tts = 'А у тебя есть волшебная фигурка?';
			$current = 'activation1';
			return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'],["title" => "Нет", "hide" => true, "payload" => 'нет'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		} else {
			$text = 'Хочешь пройти тест ещё раз или хочешь закончить?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/d83067d7-e5a5-44b1-9521-dc44a86d32dd.opus">';
			$current = 'neznakom';
			return out($text, ["buttons" => [["title" => "Пройти тест", "hide" => true, "payload" => 'пройти тест'],["title" => "Закончить", "hide" => true, "payload" => 'закончить'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);
		}
    }

	if(strpos($current, 'neznakom') !== false){
		if(strpos($command, 'законч') !== false){
			$text = 'Хорошо, возвращайся, когда захочешь продолжить';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/857e8b1f-ed36-464e-baba-45abe09a3c4b.opus">';
			$current = 'neznakom';
			return out($text, [], $tts, true);
		} else {
			$text = 'Отлично. Тебе нужно будет ответить на пять вопросов. А если сомневаешься, скажи «Повтори вопрос». Договорились?';     
			$tts = '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/1554c5e1-d399-4dc7-85d8-48f5b180cdd2.opus">';
			$current = 'start2';
			return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'],["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts);	
		}
	}

	if(strpos($current, 'activation3') !== false){
		if (strpos($event['request']['type'], 'SkillProduct.Activated') !== false) {
			if (strpos($event['request']['product_uuid'], '38b7d4b6-6c27-456f-be10-ae1d9e23ae6b') !== false) {
				$include = 'anna.php';
				goto scene;
            } else if (strpos($event['request']['product_uuid'], '321e4f75-3234-4f8b-94c2-4cf2bd9cd147') !== false) {
				$include = 'elza.php';
				goto scene;
            } else if (strpos($event['request']['product_uuid'], '6844b936-e413-4cef-9e1e-3a3a08649cd4') !== false) {
				$include = 'olaf.php';
				goto scene;
            }
		} else {
			
			$current = 'wrong-activation';
			$text = 'Ой. Ворота в Эренделл что-то не открываются. А у тебя точно есть фигурка?';
			$tts = 'Ой. Ворота в Эренделл что-то не открываются. А у тебя точно есть фигурка?';
			return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'], ["title" => "Нет", "hide" => true, "payload" => 'нет']]], $tts);
		
		}
	}

	
	if(strpos($current, 'wrong-activation') !== false){
		if(strpos($command, 'да') !== false || strpos($command, 'есть') !== false){
			$text = 'Отлично, тогда поднеси её поближе и нажми на подставку.';
			$tts = 'Отлично, тогда поднеси её поближе и нажми на подставку.';
			$current = 'activation3';
			return out($text, ["buttons" => [["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts, false, "music");
		} else {
			$text = 'Жаль. Без фигурки мы не сможем отправиться в Эренделл. Возвращайся скорее вместе с ней.';     
			$tts = 'Жаль. Без фигурки мы не сможем отправиться в Эренделл. Возвращайся скорее вместе с ней.';
			return out($text, ["buttons" => [["title" => "С начала", "hide" => true, "payload" => 'сначала']]], $tts, true);	
		}
	}


	if(strpos($current, 'test-result') !== false){
scene:
		$current = 'test-result';
		$test = 'done';

		include '/function/code/'.$include;

		if($scene == null){
			$scene = '0';
		}

		$c = intval($scene);
		if(strpos($current, 'zahod') === false && $event['session']['new'] == false){
			foreach ($s[$c]["ans"] as $n => $a) {
				$empty = false;
				if ($a === '') {
					$empty = true;
				}
				if($empty || strpos($command, $a) !== false){
					$scene = $n;
					$c = $n;
					break;
					goto exid;	
				}
			}
		} else {
			$c = intval($scene);
		}
		foreach ($s[$c]["b"] as $i) {
			$bttns[] = ["title" => $i, "hide" => true, "payload" => $i];
		}
		
		if($include == 'olaf.php' && ($scene == 43 || $scene == 44)){
			$image = '1030494/0ffc383bb335c0a9621d';
			return out($s[$c]["txt"], ['fimg' => [
				"type" => "BigImage",
				"image_id" => $image,
				"description" => $s[$c]["txt"], 
				"button" => ["url" => 'https://yandex.ru/alice/frozen/frozen-all']
			], "buttons" => [["title" => 'Поделиться', "hide" => true, "url" => 'https://yandex.ru/alice/frozen/frozen-all'], ["title" => "С начала", "hide" => true, "payload" => "сначала"]]], $s[$c]["tts"]);
		}
		if($include == 'anna.php' && ($scene == 39 || $scene == 40)){
			$image = '1030494/0ffc383bb335c0a9621d';
			return out($s[$c]["txt"], ['fimg' => [
				"type" => "BigImage",
				"image_id" => $image,
				"description" => $s[$c]["txt"], 
				"button" => ["url" => 'https://yandex.ru/alice/frozen/frozen-all']
			], "buttons" => [["title" => 'Поделиться', "hide" => true, "url" => 'https://yandex.ru/alice/frozen/frozen-all'], ["title" => "С начала", "hide" => true, "payload" => "сначала"]]], $s[$c]["tts"]);
		}
		if($include == 'elza.php' && ($scene == 58 || $scene == 59)){
			$image = '1030494/0ffc383bb335c0a9621d';
			return out($s[$c]["txt"], ['fimg' => [
				"type" => "BigImage",
				"image_id" => $image,
				"description" => $s[$c]["txt"], 
				"button" => ["url" => 'https://yandex.ru/alice/frozen/frozen-all']
			], "buttons" => [["title" => 'Поделиться', "hide" => true, "url" => 'https://yandex.ru/alice/frozen/frozen-all'], ["title" => "С начала", "hide" => true, "payload" => "сначала"]]], $s[$c]["tts"]);
		}

		$bttns[] = ["title" => "С начала", "hide" => true, "payload" => 'сначала'];
exid:

		return out($addtext.$s[$c]["txt"], ["img" => $s[$c]["img"], "buttons" => $bttns], $addtts.$s[$c]["tts"]);
	}


return out('Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".', [], 'Чтобы узнать, кто твой герой "Холодного сердца", скажи "герой". А если хочешь сразу отправиться в путешествие в Эренделл — скажи "путешествие".');
}
?>