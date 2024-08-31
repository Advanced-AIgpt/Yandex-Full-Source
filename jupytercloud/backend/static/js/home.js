require(["jquery", "jhapi"], function(
  $,
  JHAPI,
) {
    "use strict";

    let base_url = window.jhdata.base_url;
    let prefix = window.jhdata.prefix;
    let user = window.jhdata.user;
    let api = new JHAPI(base_url);

    $("#stop-server").click(function() {
        $("#stop-server-dialog").modal();
    });

    $(".stop-button").click(function() {
        $(".server-alive")
            .attr("disabled", true)
            .attr("title", "Your server is stopping")
            .click(function() {
                return false;
            });

        api.stop_server(user, {
            success: function() {
               $("#start")
                  .text("Start My Server")
                  .attr("title", "Start your default server")
                  .attr("disabled", false)
                  .show()
                  .off("click");

               $("#stop-server").fadeOut();
               $("#jupyterlab").fadeOut();
               $("#frontend-dropdown").fadeOut();
               $("#qyp").fadeOut();
               $("#server-host-row").fadeOut();
            },
        });
    });

    $("#server-host-button").click(function() {
        let target = $("#server-host-input")[0];
        let currentFocus = document.activeElement;

        target.focus();
        target.setSelectionRange(0, target.value.length);

        let succeed;
        try {
            succeed = document.execCommand("copy");
        } catch(e) {
            succeed = false;
        }

        if (currentFocus && typeof currentFocus.focus === "function") {
            currentFocus.focus();
        }

        return succeed;
    });
});
