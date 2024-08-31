# Доступ к капча-серверу на Perl

Доступ к капча-серверу возможен на Perl с помощью библиотеки YMP::Captcha. Исходный код библиотеки содержится в репозитории по адресу [https://svn.yandex.ru/websvn/wsvn/ymp/trunk/lib/YMP/Captcha.pm](https://svn.yandex.ru/websvn/wsvn/ymp/trunk/lib/YMP/Captcha.pm).

Ниже показаны примеры запросов к капча-серверу на Perl с использованием библиотеки YMP::Captcha.

```perl
use Test::More qw(no_plan);
use lib '/opt/www/ymp/lib/';
use YMP::Captcha;
use LWP;

# get new image
# by default
my $image = YMP::Captcha::New();
ok( $image, 'captcha requested, url gathered');

# or with some defined parameters
$image = YMP::Captcha::New( checks => 2, type=> 'rus');
ok( $image, 'captcha requested, url gathered');

ok( $image->key(), 'image has key');
ok( $image->url(), 'image has url');

#fetch image
my $ua = LWP::UserAgent->new();
my $get = $ua->get( $image->url() );
ok( $get->is_success(), 'Image got' );
#without getting image 
#we will always get 404 on check

# check answer
my $check = YMP::Captcha::Check(
        key => $image->key(),
        rep => '111222',
    );
is( $check, 0 , 'Check failed');

$check = YMP::Captcha::Check(
        key => $image->key(), #obligatory
        rep => '111222',      #obligatory
        type=> 'std',         #recommended to 
    );

# second failed check always return 404, but with valid html    
is( $check, 0, 'Check dropped');
```

