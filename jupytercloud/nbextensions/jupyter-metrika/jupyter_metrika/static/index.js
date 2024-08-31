define(['jquery'], function($) {
    function get_username() {
        const loc = new URL(window.location);
        return loc.pathname.split("/")[2];
    }

    function insert_metrika() {
        const metrika_counter = 64806643;
        return $(`
<!-- Yandex.Metrika counter -->
<script type="text/javascript" >
   (function(m,e,t,r,i,k,a){m[i]=m[i]||function(){(m[i].a=m[i].a||[]).push(arguments)};
   m[i].l=1*new Date();k=e.createElement(t),a=e.getElementsByTagName(t)[0],k.async=1,k.src=r,a.parentNode.insertBefore(k,a)})
   (window, document, "script", "https://mc.yandex.ru/metrika/tag.js", "ym");

   ym(${metrika_counter}, "init", {
        clickmap:true,
        trackLinks:true,
        accurateTrackBounce:true
   });
</script>
<noscript><div><img src="https://mc.yandex.ru/watch/64806643" style="position:absolute; left:-9999px;" alt="" /></div></noscript>
<!-- /Yandex.Metrika counter -->
        `).appendTo($('body'));
    }

    function insert_userid_script(user) {
        return $(`
<script>
    (function () {
        ym(64806643, 'userParams', {
            UserID: "${user}"
        });
    })();
</script>`).appendTo($('body'));
    }

    function load_ipython_extension(){
        const username = get_username();
        insert_metrika();
        insert_userid_script(username);
    }

    return {
        load_ipython_extension: load_ipython_extension
    };
});

