BUILD_ONLY_IF(OS_LINUX ARCH_X86_64)
DATA(sbr://3213815484)  # traefik

DEPENDS(jupytercloud/backend/tests/mock/app)
DEPENDS(jupytercloud/backend/tests/mock/recipe)
USE_RECIPE(jupytercloud/backend/tests/mock/recipe/mock_launcher_recipe)

PEERDIR(jupytercloud/backend/tests/mock/spec)
