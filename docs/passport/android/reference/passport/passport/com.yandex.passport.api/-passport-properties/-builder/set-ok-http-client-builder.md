//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportProperties](../index.md)/[Builder](index.md)/[setOkHttpClientBuilder](set-ok-http-client-builder.md)

# setOkHttpClientBuilder

[passport]\
abstract fun [setOkHttpClientBuilder](set-ok-http-client-builder.md)(okHttpClientBuilder: OkHttpClient.Builder): [PassportProperties.Builder](index.md)

Определить OkHttpClient.Builder. Удобно использовать для отладки и при необходимости переопределить компоненты сетевого уровня.<br></br><br></br> `<pre> OkHttpClient.Builder okHttpClientBuilder = new OkHttpClient.Builder() .dns(hostname -> { return Dns.SYSTEM.lookup(hostname); }) .addNetworkInterceptor(new HttpLoggingInterceptor(message -> Log.d(&quot;Network&quot;, message)) .setLevel(HttpLoggingInterceptor.Level.BODY)) .addNetworkInterceptor(new StethoInterceptor());

</pre>` *

#### Return

[Builder](index.md)
