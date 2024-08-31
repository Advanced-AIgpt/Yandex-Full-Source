Подпатченный asan_symbolize, работает с нашим бинарем.
Использовать так:
 * записываем вывод санитайзера в файл (скажем **output.log**)
 * заходим в директорию с бинарем: и запускаем `$PATH_TO_ASAN_SYMBOLIZE -d $STRIP < output.log`
   $STRIP - полный путь к бинарнику, там где он лежал когда упал
   -d - это показываеть нормальные человекочитаемые имена функций

Пример:

допустим на машине бинарник лежит тут: /place/db/iss3/instances/23460_testing_personal_cards_gs9OMRz0DPE/personal_cards
лог у нас будет такой:
```
=================================================================
==2484==ERROR: AddressSanitizer: heap-use-after-free on address 0x603001041cb8 at pc 0x000000287727 bp 0x7ff6eb3ee650 sp 0x7ff6eb3ede00
READ of size 3 at 0x603001041cb8 thread T133
    #0 0x287726  (/place/db/iss3/instances/23460_testing_personal_cards_gs9OMRz0DPE/personal_cards+0x287726)
    #1 0x39b256  (/place/db/iss3/instances/23460_testing_personal_cards_gs9OMRz0DPE/personal_cards+0x39b256)
```
запускаем так: `$PATH_TO_ASAN_SYMBOLIZE -d /place/db/iss3/instances/23460_testing_personal_cards_gs9OMRz0DPE/ < logfile.log`
вывод будет такой
```
=================================================================
==2484==ERROR: AddressSanitizer: heap-use-after-free on address 0x603001041cb8 at pc 0x000000287727 bp 0x7ff6eb3ee650 sp 0x7ff6eb3ede00
READ of size 3 at 0x603001041cb8 thread T133
    #0 0x287726 in __asan_memcpy _asan_rtl_ (discriminator 83)
    #1 0x39b256 in _ZL7MemCopyIcENSt4__y19enable_ifIXsr11TTypeTraitsIT_EE5IsPodEPS2_E4typeES3_PKS2_m /place/db-0/key_0/user-0/svnc2/1/root/4ac544466abf4ec69ea4c9b47272fff3/rev_3272833_0xF968/util/generic/mem_copy.h:18
    #2 0x39b256 in NCharTraitsImpl::TMutable<char>::Copy(char*, char const*, unsigned long) /place/db-0/key_0/user-0/svnc2/1/root/4ac544466abf4ec69ea4c9b47272fff3/rev_3272833_0xF968/util/generic/chartraits.h:334
    #3 0x39b256 in TBasicStringBuf<char, TCharTraits<char> > TMemoryPool::AppendCString<char>(TBasicStringBuf<char, TCharTraits<char> > const&) /place/db-0/key_0/user-0/svnc2/1/root/4ac544466abf4ec69ea4c9b47272fff3/rev_3272833_0xF968/util/memory/pool.h:220
    #4 0x39b256 in NSc::NDefinitions::TPool::AppendBuf(TBasicStringBuf<char, TCharTraits<char> > const&) /place/db-0/key_0/user-0/svnc2/1/root/4ac544466abf4ec69ea4c9b47272fff3/rev_3272833_0xF968/library/scheme/scimpl_defs.h:33
    #5 0x39b256 in NSc::TValue::TScCore::SetString(TIntrusivePtr<NSc::NDefinitions::TPool, TDefaultIntrusivePtrOps<NSc::NDefinitions::TPool> >&, TBasicStringBuf<char, TCharTraits<char> >) /place/db-0/key_0/user-0/svnc2/1/root/4ac544466abf4ec69ea4c9b47272fff3/rev_3272833_0xF968/library/scheme/scimpl.h:93
    #6 0x39b256 in NSc::TValue::SetString(TBasicStringBuf<char, TCharT
```
