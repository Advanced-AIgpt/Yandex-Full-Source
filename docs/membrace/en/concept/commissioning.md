# Launching stream processing

Once the project is approved and ready to launch, you can start integration via the API:

{% cut "Step 1. Sign up." %}

1. Go to the [sign-up page](https://passport.yandex.com/auth?origin=toloka_requesters&retpath=https://toloka.yandex.com/signup/requester) and create an account:

   {% cut "Using your Google account" %}

      1. In the registration window, click **Google**.

         ![Sign up using your Google account — Step 1](../_assets/login_google1.png "Select Google" =127x)

      1. Select a username and confirm account creation.

         ![Sign up using your Google account — Step 2](../_assets/login_google2.png "Confirm account creation in Toloka" =126x)

   {% endcut %}

   {% cut "Using your email" %}

      1. In the registration window, enter your email and click **Log in**.

         ![Sign up using your email — Step 1](../_assets/login_email1.png "Enter email address" =124x)

      1. You'll receive a confirmation code in your email. Enter it in the window that opens.

         ![Sign up using your email — Step 2](../_assets/login_email2.png "Enter confirmation code" =126x)

   {% endcut %}

   1. Complete registration by filling out your personal information.

     ![Complete signup](../_assets/info-about-yourself.png "Fill out personal information" =417x)

{% endcut %}

{% cut "Step 2. Top up your balance." %}

1. Top up your balance.

   1. Go to the **Profile** tab.

      ![Profile](../_assets/profile.png "Go to your profile" =630x)

   1. For a quick start, use our promo code:

   1. Click **Enter promo code**.

      ![Top up your balance using a promo code](../_assets/enter-promo-code.png "Click to enter promo code" =307x)

   1. Enter `TOLOKAKIT1`. Once it's activated, your balance will be topped up.

      ![Activate the promo code](../_assets/promocode.png "Enter and activate the promo code" =206x)

   {% note info %}

   To continue using the service, you'll need to link your account to a billing system. To learn more about linking your account, check [this page](https://toloka.ai/docs/guide/concepts/budget.html).

   {% endnote %}

{% endcut %}

{% cut "Step 3. Send us your login." %}

   Send your consultant the login that you registered with, and we'll link your ready-to-go solution to it. You can find your login in your [profile](https://toloka.yandex.com/requester/profile).

{% endcut %}

{% cut "Step 4. Name your project." %}

  1. We'll send you a link in response to your login information – follow it.

  1. Create a project:

      1. In the **Project setup** section, enter the name of your app and click **Go to instructions setup**.

         ![Project setup](../_assets/project-name.png "Enter name" =631x)

      1. You don't need to enter anything in the **Instructions setup** section. Go straight to the next section by clicking **Go to Final check**.

            ![Instruction setup](../_assets/go-final-check.png "Go to final check" =630x)

      1. In the **Final check** section, click **Create project**.

         ![Final check](../_assets/final-check.png "Create a project" =633x)

  1. You'll see your project data appear on the screen. Copy the project ID and send it to your consultant.

        ![Project data](../_assets/project-id.png "Copy project ID" =629x)

  1. Wait for the project to activate. You can see the status info on the page you copied the ID from.

     ![Project status](../_assets/project-activated.png "Project activated" =634x)

{% endcut %}

{% cut "Step 5. Connect the API and get started." %}

   1. Get an authorization token: on the **Integrations** tab in your [profile](https://toloka.yandex.com/requester/profile/integration), click **Get OAuth token**.

      ![OAuth-token](../_assets/get-oauth-token.png "Get a token" =626x)

   1. You are now ready to exchange data via the API — send your project data and receive moderation results:

      - Use stream data processing (not batch processing). To learn more, see [Help](https://toloka.ai/ru/docs/toloka-apps/api/concepts/streaming-items.html).

      - You'll need the project ID that you got in the previous step. In Help, the project ID corresponds to the `{app_project_id}` variable.

      {% note info %}

      To learn more about the API, check the following pages:
      - [Getting project information](https://toloka.ai/ru/docs/toloka-apps/api/ref/app-project/app-projects_app_project_id_get.html)
      - [Getting labeling item information](https://toloka.ai/ru/docs/toloka-apps/api/ref/item/app-projects_app_project_id_items_item_id_get.html)
      - [Getting a list of all project items](https://toloka.ai/ru/docs/toloka-apps/api/ref/item/app-projects_app_project_id_items_get.html)

      {% endnote %}

{% endcut %}

