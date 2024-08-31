<?php

$current = "";
$prev_com = "";
$prev_cur = "";
$include = "";
$user = "";
$crosh = "";
$nyusha = "";
$ezhik = "";

function out($text, $opt = array(), $tts = false, $end_session = false, $activationtype = null){
    global $current, $prev_com, $prev_cur, $include, $user, $crosh, $nyusha, $ezhik;
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
                'include' => $include
            ],
            'user_state_update' => [
                'value' => $user,
                'crosh' => $crosh,
                'nyusha' => $nyusha,
                'ezhik' => $ezhik
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
                'include' => $include
            ],
            'user_state_update' => [
                'value' => $user,
                'crosh' => $crosh,
                'nyusha' => $nyusha,
                'ezhik' => $ezhik
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
	
		global $current, $prev_com, $prev_cur, $include, $user, $crosh, $nyusha, $ezhik;
		$current = $event['state']['session']['current'];
		$prev_com = $event['state']['session']['prev_com'];
        $prev_cur = $event['state']['session']['prev_cur'];
        $include = $event['state']['session']['include'];

        $user = $event['state']['user']['value'];
        
        $crosh = $event['state']['user']['crosh'];
        $nyusha = $event['state']['user']['nyusha'];
        $ezhik = $event['state']['user']['ezhik'];

		$u = $event['session']['user_id'];
		$command = mb_strtolower($event['request']['command'].$event['request']['payload'], 'UTF-8');
	}


    $split = explode(" ", $command);
    $split = $split[count($split)-1];

    if(strpos($command, 'познакомься') !== false || strpos($command, 'активир') !== false){
        if (strpos($command, 'кр') !== false) {

                    
            $found = false;
    
            foreach($event['session']['user']['skill_products'] as $sp){
                if(strpos($sp['uuid'], '7cc16ab6-ecef-11ea-adc1-0242ac120002') !== false) {
                    $found = true;
                }
            }
                if ($found) {
                   
                    if($crosh !== 0 && $crosh !== null){
                        $include = 'crosh.php';
                        $text = 'Хотите начать игру заново или продолжить?';
                        $tts = $text;
                        $current = 'neworcontinue';
                        return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                    }else{
                        $include = 'crosh.php';
                        goto question;
                    }

                } else {
                    if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                        $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                        $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                        return out($text, [], $tts, true);
                    }
                    $text = 'Давай! Можешь нажать на лапку Смешарика?';
                    $tts = $text;
                    $current = 'raspoznaem';
        
                    return out($text, [] , $tts, false, "music");

                }
        } else if (strpos($command, 'ню') !== false) {
            $found = false;
    
            foreach($event['session']['user']['skill_products'] as $sp){
                if(strpos($sp['uuid'], 'c58e86a2-e6c4-4bf0-bc93-f9e3d5068458') !== false) {
                    $found = true;
                }
            }
                if ($found) {
                   
                    if($nyusha !== 0 && $nyusha !== null){
                        $include = 'nyusha.php';
                        $text = 'Хотите начать игру заново или продолжить?';
                        $tts = $text;
                        $current = 'neworcontinue';
                        return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                    }else{
                        $include = 'nyusha.php';
                        goto question;
                    }

                } else {
                    if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                        $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                        $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                        return out($text, [], $tts, true);
                    }
                    $text = 'Давай! Можешь нажать на лапку Смешарика?';
                    $tts = $text;
                    $current = 'raspoznaem';
        
                    return out($text, [] , $tts, false, "music");

                }
        } else if (strpos($command, 'еж') !== false) {
            $found = false;
    
            foreach($event['session']['user']['skill_products'] as $sp){
                if(strpos($sp['uuid'], 'a0a2d12c-ecef-11ea-adc1-0242ac120002') !== false) {
                    $found = true;
                }
            }
                if ($found) {
                   
                    if($ezhik !== 0 && $ezhik !== null){
                        $include = 'ezhik.php';
                        $text = 'Хотите начать игру заново или продолжить?';
                        $tts = $text;
                        $current = 'neworcontinue';
                        return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                    }else{
                        $include = 'ezhik.php';
                        goto question;
                    }

                } else {
                    if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                        $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                        $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                        return out($text, [], $tts, true);
                    }
                    $text = 'Давай! Можешь нажать на лапку Смешарика?';
                    $tts = $text;
                    $current = 'raspoznaem';
        
                    return out($text, [] , $tts, false, "music");

                }
        } else {
            
            $text = 'С кем бы вы хотели поиграть?';
            $tts = $text;
            $current = 'vybor';
            return out($text, ["buttons" => [["title" => "Крош", "hide" => true, "payload" => 'крош'],["title" => "Ежик", "hide" => true, "payload" => 'ежик'],["title" => "Нюша", "hide" => true, "payload" => 'нюша']]], $tts);

        }
    }


    if($event['session']['new'] == true || $current === 'endgame'){
        
        if(strpos($command, 'крош') !== false || strpos($command, 'еж') !== false || strpos($command, 'нюш') !== false){
            if (strpos($command, 'кр') !== false) {

                    
                $found = false;
        
                foreach($event['session']['user']['skill_products'] as $sp){
                    if(strpos($sp['uuid'], '7cc16ab6-ecef-11ea-adc1-0242ac120002') !== false) {
                        $found = true;
                    }
                }
                    if ($found) {
                       
                        if($crosh !== 0 && $crosh !== null){
                            $include = 'crosh.php';
                            $text = 'Хотите начать игру заново или продолжить?';
                            $tts = $text;
                            $current = 'neworcontinue';
                            return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                        }else{
                            $include = 'crosh.php';
                            goto question;
                        }

                    } else {
                        
                        $text = 'Я умею играть только с игрушкой Смешарика, у тебя есть игрушка?';
                        $tts = $text;
                        $current = 'nachalo';
                        return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'], ["title" => "Нет", "hide" => true, "payload" => 'нет']]], $tts);
                    }
            } 
            
            if (strpos($command, 'ню') !== false) {
                $found = false;
        
                foreach($event['session']['user']['skill_products'] as $sp){
                    if(strpos($sp['uuid'], 'c58e86a2-e6c4-4bf0-bc93-f9e3d5068458') !== false) {
                        $found = true;
                    }
                }
                    if ($found) {
                       
                        if($nyusha !== 0 && $nyusha !== null){
                            $include = 'nyusha.php';
                            $text = 'Хотите начать игру заново или продолжить?';
                            $tts = $text;
                            $current = 'neworcontinue';
                            return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                        }else{
                            $include = 'nyusha.php';
                            goto question;
                        }

                    } else {
                        
                        $text = 'Я умею играть только с игрушкой Смешарика, у тебя есть игрушка?';
                        $tts = $text;
                        $current = 'nachalo';
                        return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'], ["title" => "Нет", "hide" => true, "payload" => 'нет']]], $tts);
                    }
            } 
            
            if (strpos($command, 'еж') !== false) {
                $found = false;
        
                foreach($event['session']['user']['skill_products'] as $sp){
                    if(strpos($sp['uuid'], 'a0a2d12c-ecef-11ea-adc1-0242ac120002') !== false) {
                        $found = true;
                    }
                }
                    if ($found) {
                       
                        if($ezhik !== 0 && $ezhik !== null){
                            $include = 'ezhik.php';
                            $text = 'Хотите начать игру заново или продолжить?';
                            $tts = $text;
                            $current = 'neworcontinue';
                            return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                        }else{
                            $include = 'ezhik.php';
                            goto question;
                        }

                    } else {
                        
                        $text = 'Я умею играть только с игрушкой Смешарика, у тебя есть игрушка?';
                        $tts = $text;
                        $current = 'nachalo';
                        return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'], ["title" => "Нет", "hide" => true, "payload" => 'нет']]], $tts);
                    }
            }
        }
        
        $gamescount = 0;

        


        foreach($event['session']['user']['skill_products'] as $sp){
            if (strpos($sp['uuid'], '7cc16ab6-ecef-11ea-adc1-0242ac120002') !== false) {
                $text = $text.'Крошем';
                $gamescount++;
            }
            if (strpos($sp['uuid'], 'a0a2d12c-ecef-11ea-adc1-0242ac120002') !== false) {
                $text = $text.'Ежиком';
                $gamescount++;
            }
            if (strpos($sp['uuid'], 'c58e86a2-e6c4-4bf0-bc93-f9e3d5068458') !== false) {
                $text = $text.' Нюшей';
                $gamescount++;
            }

        }

        if ($gamescount == 0) {

            $text = 'Я умею играть только с игрушкой Смешарика, у тебя есть игрушка?';
            $tts = $text;
            $current = 'nachalo';
            return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'], ["title" => "Нет", "hide" => true, "payload" => 'нет']]], $tts);
        }
        

        if ($gamescount == 1) {
            
            if ($text == 'Крошем') {
                $include = 'crosh.php';
            } elseif ($text == 'Ежиком') {
                $include = 'ezhik.php';
            } else {
                $include = 'nyusha.php';
            }
            
            $text = 'Продолжим играть с '.trim($text).'?';
            $tts = $text;
            

            $current = 'prodolzhaem';


            return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'], ["title" => "Нет", "hide" => true, "payload" => 'нет']]], $tts);
        }


        if ($gamescount == 2) {
            
        
            $game1 = mb_substr($text, 6);
            $game2 = mb_substr($text, 0, 6);

            
            $text = 'Давай! Можем поиграть с '.trim($game1).' или '.trim($game2).'.';
            $tts = $text;

            $current = 'vybor';
            return out($text, [] , $tts);
        }

        if ($gamescount == 3) {
            $text = 'Давай! Можем поиграть с Крошем, Ежиком или Нюшей.';
            $tts = $text;
            $current = 'vybor';
            return out($text, ["buttons" => [["title" => "Крош", "hide" => true, "payload" => 'крош'],["title" => "Ежик", "hide" => true, "payload" => 'ежик'],["title" => "Нюша", "hide" => true, "payload" => 'нюша']]], $tts);
        }
    }

    if (strpos($current, 'nachalo') !== false) {
        if (strpos($command, 'да') !== false || strpos($command, 'есть') !== false) {
            if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                return out($text, [], $tts, true);
            }
            $text = 'Отлично! Можешь нажать на лапку Смешарика?';
            $tts = $text;
            $current = 'raspoznaem';
            return out($text, [] , $tts, false, "music");            
        } else {
            $text = 'К сожалению, я пока без игрушки не умею. Возвращайся с игрушкой - и мы обязательно сыграем.';
            $tts = 'К сожалению, я пока без игрушки не умею. Возвращайся с игрушкой - и мы обязательно сыграем.';
            $link = 'https://dialogs.yandex.ru/store/skills/955a5db8-kvest-pro-dinozavrov';
            return out($text, ["buttons" => [["title" => 'Смешарики в мире динозавров', "hide" => false, "url" => $link]]], $tts, true);  
        }
    }


    if (strpos($current, 'raspoznaem') !== false) {

        if (strpos($event['request']['type'], 'SkillProduct.Activated') !== false) {
            
            if (strpos($event['request']['product_uuid'], '7cc16ab6-ecef-11ea-adc1-0242ac120002') !== false) {
                $text = 'Крошем';
                $include = 'crosh.php';
            } else if (strpos($event['request']['product_uuid'], 'a0a2d12c-ecef-11ea-adc1-0242ac120002') !== false) {
                $text = 'Ежиком';
                $include = 'ezhik.php';
            } else if (strpos($event['request']['product_uuid'], 'c58e86a2-e6c4-4bf0-bc93-f9e3d5068458') !== false) {
                $text = 'Нюшей'; 
                $include = 'nyusha.php';
            } else {
                $text = 'Давай! Можешь нажать на лапку Смешарика?';
                $tts = $text;
                $current = 'raspoznaem';

                return out($text, [] , $tts, false, "music");
            }
            
            $nachinaem = 'Слышу-слышу! Отлично! Начинаем игру с '.$text.'.';
            goto question;
        } else {
        
			$current = 'wrong-activation';
			$text = 'Ой! Не получается запустить игру. Проверь, у тебя есть игрушка?';
			$tts = 'Ой! Не получается запустить игру. Проверь, у тебя есть игрушка?';
			return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да'], ["title" => "Нет", "hide" => true, "payload" => 'нет']]], $tts);
        }
    }

    
	if(strpos($current, 'wrong-activation') !== false){
		if(strpos($command, 'да') !== false || strpos($command, 'есть') !== false){
			$text = 'Отлично! Можешь нажать на лапку Смешарика?';
			$tts = 'Отлично! Можешь нажать на лапку Смешарика?';
			$current = 'raspoznaem';
			return out($text, [], $tts, false, "music");
		} else {
            $text = 'К сожалению, я пока без игрушки не умею. Возвращайся с игрушкой - и мы обязательно сыграем.';
            $tts = 'К сожалению, я пока без игрушки не умею. Возвращайся с игрушкой - и мы обязательно сыграем.';
            $link = 'https://dialogs.yandex.ru/store/skills/955a5db8-kvest-pro-dinozavrov';
            return out($text, ["buttons" => [["title" => 'Смешарики в мире динозавров', "hide" => false, "url" => $link]]], $tts, true); 	
		}
	}


    if (strpos($current, 'prodolzhaem') !== false && strpos($command, 'да') !== false) {
        
        if(strpos($include, 'crosh.php') !== false){
            if($crosh !== 0 && $crosh !== null){
                $include = 'crosh.php';
                $text = 'Хотите начать игру заново или продолжить?';
                $tts = $text;
                $current = 'neworcontinue';
                return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
            }else{
                $include = 'crosh.php';
                goto question;
            }
        }
        
        if(strpos($include, 'nyusha.php') !== false){
            if($nyusha !== 0 && $nyusha !== null){
                $include = 'nyusha.php';
                $text = 'Хотите начать игру заново или продолжить?';
                $tts = $text;
                $current = 'neworcontinue';
                return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
            }else{
                $include = 'nyusha.php';
                goto question;
            }
        }
        
        if(strpos($include, 'ezhik.php') !== false){
            if($ezhik !== 0 && $ezhik !== null){
                $include = 'ezhik.php';
                $text = 'Хотите начать игру заново или продолжить?';
                $tts = $text;
                $current = 'neworcontinue';
                return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
            }else{
                $include = 'ezhik.php';
                goto question;
            }
        }

    }



    if (strpos($current, 'prodolzhaem') !== false && strpos($command, 'нет') !== false) {
        if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
            $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
            $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
            return out($text, [], $tts, true);
        }
        if (strpos($command, 'нюш') !== false) {
            $text = 'Давай! Можешь нажать на лапку Смешарика?';
            $tts = $text;
            $current = 'raspoznaem';

            return out($text, [] , $tts, false, "music");
        } elseif (strpos($command, 'жик') !== false) {
            $text = 'Давай! Можешь нажать на лапку Смешарика?';
            $tts = $text;
            $current = 'raspoznaem';

            return out($text, [] , $tts, false, "music");
        } elseif (strpos($command, 'крош') !== false) {
            $text = 'Давай! Можешь нажать на лапку Смешарика?';
            $tts = $text;
            $current = 'raspoznaem';

            return out($text, [] , $tts, false, "music");
        } else {
            $text = 'С кем бы вы хотели поиграть?';
            $tts = $text;
            $current = 'vybor';
            return out($text, ["buttons" => [["title" => "Крош", "hide" => true, "payload" => 'крош'],["title" => "Ежик", "hide" => true, "payload" => 'ежик'],["title" => "Нюша", "hide" => true, "payload" => 'нюша']]], $tts);
        }
    } 


    if (strpos($current, 'vybor') !== false) {

        if (strpos($command, 'кр') !== false) {
            
            $found = false;
            
            foreach($event['session']['user']['skill_products'] as $sp){
                if(strpos($sp['uuid'], '7cc16ab6-ecef-11ea-adc1-0242ac120002') !== false) {
                    $found = true;
                }
            }
                if ($found) {
                    
                    if($crosh !== 0 && $crosh !== null){
                        $include = 'crosh.php';
                        $text = 'Хотите начать игру заново или продолжить?';
                        $tts = $text;
                        $current = 'neworcontinue';
                        return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                    }else{
                        $include = 'crosh.php';
                        goto question;
                    }

                } else {
                    if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                        $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                        $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                        return out($text, [], $tts, true);
                    }
                    $text = 'Давай! Можешь нажать на лапку Смешарика?';
                    $tts = $text;
                    $current = 'raspoznaem';
        
                    return out($text, [] , $tts, false, "music");
                }
        } else if (strpos($command, 'еж') !== false) {

            $found = false;
            
            foreach($event['session']['user']['skill_products'] as $sp){
                if(strpos($sp['uuid'], 'a0a2d12c-ecef-11ea-adc1-0242ac120002') !== false) {
                    $found = true;
                }
            }
                if ($found) {
                    
                    if($ezhik !== 0 && $ezhik !== null){
                        $include = 'ezhik.php';
                        $text = 'Хотите начать игру заново или продолжить?';
                        $tts = $text;
                        $current = 'neworcontinue';
                        return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                    }else{
                        $include = 'ezhik.php';
                        goto question;
                    }

                } else {
                    if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                        $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                        $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                        return out($text, [], $tts, true);
                    }
                    $text = 'Давай! Можешь нажать на лапку Смешарика?';
                    $tts = $text;
                    $current = 'raspoznaem';
        
                    return out($text, [] , $tts, false, "music");
                }          
        } else if (strpos($command, 'ню') !== false) {

            $found = false;
            
            foreach($event['session']['user']['skill_products'] as $sp){
                if(strpos($sp['uuid'], 'c58e86a2-e6c4-4bf0-bc93-f9e3d5068458') !== false) {
                    $found = true;
                }
            }
                if ($found) {
                    
                    if($nyusha !== 0 && $nyusha !== null){
                        $include = 'nyusha.php';
                        $text = 'Хотите начать игру заново или продолжить?';
                        $tts = $text;
                        $current = 'neworcontinue';
                        return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                    }else{
                        $include = 'nyusha.php';
                        goto question;
                    }

                } else {
                    if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                        $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                        $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                        return out($text, [], $tts, true);
                    }
                    $text = 'Давай! Можешь нажать на лапку Смешарика?';
                    $tts = $text;
                    $current = 'raspoznaem';
        
                    return out($text, [] , $tts, false, "music");
                }

        } else {
            $text = 'Назови имя Смешарика еще раз.';
            $current = 'vybor';
            $tts = $text;

            return out($text, ["buttons" => [["title" => "Крош", "hide" => true, "payload" => 'крош'],["title" => "Ежик", "hide" => true, "payload" => 'ежик'],["title" => "Нюша", "hide" => true, "payload" => 'нюша']]], $tts);
        }
    }


    if (strpos($command, 'хочу поиграть') !== false) {

                if (strpos($command, 'кр') !== false) {

                    
                    $found = false;
            
                    foreach($event['session']['user']['skill_products'] as $sp){
                        if(strpos($sp['uuid'], '7cc16ab6-ecef-11ea-adc1-0242ac120002') !== false) {
                            $found = true;
                        }
                    }
                        if ($found) {
                           
                            if($crosh !== 0 && $crosh !== null){
                                $include = 'crosh.php';
                                $text = 'Хотите начать игру заново или продолжить?';
                                $tts = $text;
                                $current = 'neworcontinue';
                                return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                            }else{
                                $include = 'crosh.php';
                                goto question;
                            }

                        } else {
                            if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                                $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                                $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                                return out($text, [], $tts, true);
                            }
                            $text = 'Давай! Можешь нажать на лапку Смешарика?';
                            $tts = $text;
                            $current = 'raspoznaem';
                
                            return out($text, [] , $tts, false, "music");

                        }
                } else if (strpos($command, 'ню') !== false) {
                    $found = false;
            
                    foreach($event['session']['user']['skill_products'] as $sp){
                        if(strpos($sp['uuid'], 'c58e86a2-e6c4-4bf0-bc93-f9e3d5068458') !== false) {
                            $found = true;
                        }
                    }
                        if ($found) {
                           
                            if($nyusha !== 0 && $nyusha !== null){
                                $include = 'nyusha.php';
                                $text = 'Хотите начать игру заново или продолжить?';
                                $tts = $text;
                                $current = 'neworcontinue';
                                return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                            }else{
                                $include = 'nyusha.php';
                                goto question;
                            }

                        } else {
                            if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                                $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                                $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                                return out($text, [], $tts, true);
                            }
                            $text = 'Давай! Можешь нажать на лапку Смешарика?';
                            $tts = $text;
                            $current = 'raspoznaem';
                
                            return out($text, [] , $tts, false, "music");

                        }
                } else if (strpos($command, 'еж') !== false) {
                    $found = false;
            
                    foreach($event['session']['user']['skill_products'] as $sp){
                        if(strpos($sp['uuid'], 'a0a2d12c-ecef-11ea-adc1-0242ac120002') !== false) {
                            $found = true;
                        }
                    }
                        if ($found) {
                           
                            if($ezhik !== 0 && $ezhik !== null){
                                $include = 'ezhik.php';
                                $text = 'Хотите начать игру заново или продолжить?';
                                $tts = $text;
                                $current = 'neworcontinue';
                                return out($text, ["buttons" => [["title" => "Заново", "hide" => true, "payload" => 'заново'],["title" => "Продолжить", "hide" => true, "payload" => 'продолжить']]], $tts);
                            }else{
                                $include = 'ezhik.php';
                                goto question;
                            }

                        } else {
                            if($event['session']['user']['user_id'] === "" || $event['session']['user']['user_id'] === null){
                                $text = 'Для активации игрушки нужно авторизоваться в Яндексе';     
                                $tts = 'Для активации игрушки нужно авторизоваться в Яндексе';
                                return out($text, [], $tts, true);
                            }
                            $text = 'Давай! Можешь нажать на лапку Смешарика?';
                            $tts = $text;
                            $current = 'raspoznaem';
                
                            return out($text, [] , $tts, false, "music");

                        }
                } else {
                    
                    $text = 'С кем бы вы хотели поиграть?';
                    $tts = $text;
                    $current = 'vybor';
                    return out($text, ["buttons" => [["title" => "Крош", "hide" => true, "payload" => 'крош'],["title" => "Ежик", "hide" => true, "payload" => 'ежик'],["title" => "Нюша", "hide" => true, "payload" => 'нюша']]], $tts);

                }
    
    
    }
 

    if((strpos($command, 'повтор') !== false && strpos($current, 'help') !== false) || strpos($command, 'помощь') !== false || strpos($command, 'что ты умеешь') !== false || strpos($command, 'помоги') !== false || strpos($command, 'что умее') !== false){
		if(strpos($current, 'questq') !== false){
            $text = 'Помоги Смешарикам в приключении и громко отвечай на вопросы.
Продолжим?';
		}else{
            $text = 'Описание функционала навыка';
        }
		if (strpos($current, 'help') === false){
			$current = 'help'.$current;	
		}
        return out($text, ["buttons" => [["title" => "Да", "hide" => true, "payload" => 'да']]]);
    }
    
    $prodol = false;
    
	if(strpos($command, 'повтор') !== false || strpos($current, 'help') !== false){
        $prodol = true;
        $current = $prev_cur;
		$command = $prev_com;
	}else{
		$prev_cur = $current;
		$prev_com = $command;
	}

    if(strpos($command, 'выйти') !== false){
        return out('Будем ждать вас снова!', [], 'Будем ждать вас снова!', true);
    }


    if(strpos($current, 'neworcontinue') !== false){
        if(strpos($command, 'заново') !== false){

            $include = $event['state']['session']['include'];
    
            if($include === 'crosh.php'){
                $crosh = 0;
            }
            if($include === 'nyusha.php'){
                $nyusha = 0;
            }
            if($include === 'ezhik.php'){
                $ezhik = 0;
            }
            
            goto question;
    
        }else{
            $include = $event['state']['session']['include'];
    


            if($include === 'crosh.php'){
                if($crosh == 16){
                    $crosh = 0;
                } else {
                    $i = $crosh;
                }
            }
            if($include === 'nyusha.php'){
                if($nyusha == 17){
                    $nyusha = 0;
                } else {
                    $i = $nyusha;
                }
            }
            if($include === 'ezhik.php'){
                if($ezhik == 15){
                    $ezhik = 0;
                } else {
                    $i = $ezhik;
                }
            }
            
            goto question;
        }
    }

        if(strpos($current, 'quest') === false || strpos($current, 'questa') !== false || strpos($command, 'поехали') !== false){
            
            $i = 0;
question:

            if($include == null){
                if(strpos($command, 'крош') !== false){
                    $include = 'crosh.php';
                    $current = 'questa';
                }
                if(strpos($command, 'жик') !== false){
                    $include = 'ezhik.php';
                    $current = 'questa';
                }
                if(strpos($command, 'нюша') !== false){
                    $include = 'nyusha.php';
                    $current = 'questa';
                }
            }

            include '/function/code/'.$include;            

            if($i === null){
                $i = 0;
            }

            $current = 'questq'.$i; 
            
            if($include === 'crosh.php'){
                $crosh = $i;
            }
            if($include === 'nyusha.php'){
                $nyusha = $i;
            }
            if($include === 'ezhik.php'){
                $ezhik = $i;
            }


            if($prodol === true){
                $reacttext = "";
                $reactzvooq = "";
            }

            $text = $reacttext.$quest[$i]['q'];
            $tts = $nachinaem.$reactzvooq.$quest[$i]['zvooq'];
            
            foreach ($quest[$i]['b'] as $f) {
                $bttns[] = ["title" => $f, "hide" => true, "payload" => $f];
            }
        
            if(!empty($quest[$i]['img'])){
                $img = ["fimg" => ["type" => "BigImage", "image_id" => $quest[$i]['img'], "title" => $quest[$i]['title'], "description" => $text], "buttons" => $bttns];
            } else {
                $img = ["buttons" => $bttns];
            }


            if($include === 'crosh.php' && strpos($current, '16') !== false){
                $current = 'endgame';
            }
            if($include === 'ezhik.php' && strpos($current, '15') !== false){
                $current = 'endgame';
            }
            if($include === 'nyusha.php' && strpos($current, '17') !== false){
                $current = 'endgame';
            }

            return out($text, $img, $tts);
        }
        if(strpos($current, 'questq') !== false){
            
            include '/function/code/'.$include;

            $i = substr($current, 6);
            $f_pos = strpos($current, 'f');
			if($f_pos > 0){
                $false = intval(substr($current, $f_pos + 1));
                $i = substr($i, 0, -2);
            }else{
				$false = 1;
			}

                $right = false;
                foreach($quest[$i]['a'] as $a){ 
                    if(strpos($command, $a) !== false){
                        $right = true; 
                    }
                }

                if($right == true){
                    $reacttext = $quest[$i]['true']['desc']."\n";
                    $reactzvooq = $quest[$i]['true']['zvooq'];
                    $i++;
                    goto question;
                }else{

                    if($false == 1){
                        $false = 2;
                        
                        foreach($quest[$i]['1false'] as $v){
                            foreach($v['a'] as $r){
                                if(empty($r) || strpos($command, $r) !== false){
                                    $current = "questq".$i."f".$false;
                                    $text = $v['desc'];
                                    $tts = $v['zvooq'];
                                    return out($text, ["buttons" => [["title" => "Подскажи", "hide" => true, "payload" => 'подскажи']]], $tts);
                                }
                            }
                        }
                    }

                    if($false == 2){
                        $false = 3;

                        if(!empty($quest[$i]['note'])){
                            if(strpos($command, 'не знаю') === false && strpos($command, 'подскажи') === false && strpos($command, 'подсказка') === false){
                                foreach($quest[$i]['2false'] as $p){
                                    $current = "questq".$i."f".$false;
                                    $text = $p['desc'];
                                    $tts = $p['zvooq'];
                                    return out($text, ["buttons" => [["title" => "Подскажи", "hide" => true, "payload" => 'подскажи']]], $tts);
                                }
                            }
                        }else{
                            foreach($quest[$i]['2false'] as $p){
                                foreach($p['a'] as $n){
                                    if(empty($n) || strpos($command, $n) !== false){
                                        $current = "questq".$i."f".$false;
                                        $text = $p['desc'];
                                        $tts = $p['zvooq'];
                                        return out($text, ["buttons" => [["title" => "Подскажи", "hide" => true, "payload" => 'подскажи']]], $tts);
                                    }
                                }
                            }
                        }    
                    }

                    if($false == 3){
                        
false3:
                        foreach($quest[$i]['3false'] as $m){
                            foreach($m['a'] as $z){
                                if(empty($z) || strpos($command, $z) !== false){
                                    $reacttext = $m['desc']."\n";
                                    $reactzvooq = $m['zvooq'];
                                    $i++;
                                    goto question;
                                }
                            }
                        }
                }
        }
    }

return out('Ой, я вас не поняла. Скажите команду "Помощь", чтобы ознакомиться с возможностями навыка.', ["buttons" => [["title" => "Помощь", "hide" => true, "payload" => 'помощь']]], 'Ой, я вас не поняла. Скажите команду "Помощь", чтобы ознакомиться с возможностями навыка.');
}
?>