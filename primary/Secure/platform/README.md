# Platform

## Porting guide

1. Copy the following generated files from `Secure` to new `/platform/<name>` folder
    - `Core` folder
    - `mx_generated.cmake` file
    - Linker script (e.g. `STM32H573XX_FLASH_S.ld`)
2. Change paths in `mx_generated.cmake`.
    - All `${CMAKE_CURRENT_SOURCE_DIR}/../` should be replaced with `${CMAKE_CURRENT_SOURCE_DIR}/platform/${PLATFORM}/../../../`

This is all done automatically by `move_generated_files.py`.

## Hooks

### Main

`Core/Src/main.c`:

```C
#include "boot_main.h"
#include "config.h"
#include "error.h"

/* USER CODE BEGIN VTOR_TABLE */
#define VTOR_TABLE_NS_START_ADDR (FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET)
/* USER CODE END VTOR_TABLE */

/* USER CODE BEGIN 2 */
boot_main();
/* USER CODE END 2 */

/* USER CODE BEGIN Error_Handler_Debug */
error_handler(ERROR_HAL, HAL_ERROR);
/* USER CODE END Error_Handler_Debug */
```

### Keys

`Core/Src/aes.c`:

```C
/* USER CODE BEGIN 0 */
#include "secrets.h"
/* USER CODE END 0 */

void MX_SAES_AES_Init(void) {

    /* USER CODE BEGIN SAES_Init 0 */
    set_saes_key(pKeySAES);
    set_saes_init_vector(pInitVectSAES);
    /* USER CODE END SAES_Init 0 */

    ...
}
```
