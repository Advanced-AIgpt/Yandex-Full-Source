# Обфускация данных 

## Генерация ключа шифрования

Для симметричного шифрования требуется ключ. В простейшем случае в библиотеке для симметричного шифрования применяется алгоритм AES с 256-битным ключом. Конкретно в этой инструкции описана процедура, для которой требуется ключ в виде строки из 64 символов, представляющих из себя корректное число в 16-ричной системе счисления. Сгенерировать его можно, например, с помощью команды `openssl enc -aes-256-cbc -k secret -P -md sha1` в терминале. Часть вывода после выполнения команды будет иметь вид:

```
salt=172D6C4ED5B603A2
key=3817769D94847B26851581E9B2C837A106DC1BAEE7B48D07CCF4236FC5CFE442
iv =3F6F4D06AEB4AA0E0D9E7316B595CFB0
```

Нам интересна строка из 64 символов, выписанная после `key=` — это и есть ключ шифрования, записанный в 16-ричной системе счисления.

## Добавление ключа шифрования в registry секретов

Выполните шаги из раздела [Хранение секретов в памяти Голливуда](../secrets.md#sekrety-v-registry-sekretov).

Далее будем считать, что ключ добавленного и зарегистрированного секрета лежит в переменной:

```
constexpr TStringBuf AES_ENCRYPTION_KEY = "AES_ENCRYPTION_KEY";
```

## Добавление либы в код

- В `ya.make` добавьте `PEERDIR` на `alice/hollywood/library/crypto`;
- Заинклюдьте:
  
  ```
  #include <alice/hollywood/library/crypto/aes.h>
  ```

## Простейшее шифрование строки

Пусть в переменной `sensitiveData` типа `TString` лежит sensitive строка, которую нужно зашифровать с помощью ранее сгенерированного ключа. Будучи внутри `namespace NAlice::NHollywood`, делаем так:

```
TString encodedSensitiveData;
Y_ENSURE(NCrypto::AESEncryptWeakWithSecret(AES_ENCRYPTION_KEY,
                                           sensitiveData,
                                           encodedSensitiveData), "Sensitive data encryption failed");
```

Если шифрование прошло успешно, в переменной `encodedSensitiveData` будут лежать данные в зашифрованном виде.

## Простейшее дешифрование строки

Обратное шифрование делается аналогичным образом:

```
TString originalSensitiveData;
Y_ENSURE(NCrypto::AESDecryptWeakWithSecret(AES_ENCRYPTION_KEY,
                                           encodedSensitiveData,
                                           originalSensitiveData), "Sensitive data decryption failed");
```
