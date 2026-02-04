# Hooks

## Non-Secure

Almost all code is contained in the Application and Libraries folders, however certain functions must interact with auto-generated code. This page lists all such scenarios so they can be reimplemented if lost due to re-auto-generation.

### Set MAC address && Ethernet DMA Descriptors

```C

#include "utils.h"

...

/* USER CODE BEGIN 0 */
__attribute__((section(".ETH"))) ETH_DMADescTypeDef CustomDMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((section(".ETH"))) ETH_DMADescTypeDef CustomDMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
/* USER CODE END 0 */

void MX_ETH_Init(void) {

    ...

    /* USER CODE BEGIN MACADDRESS */
    write_mac_addr(MACAddr);
    heth.Init.TxDesc = CustomDMATxDscrTab;
    heth.Init.RxDesc = CustomDMARxDscrTab;
    /* USER CODE END MACADDRESS */

    ...
}
```

Ethernet DMA descriptors be placed in the ETH section of RAM (marked by the MPU as shareable).

### ThreadX init

`Core/Src/app_threadx.c`:

```C
/* USER CODE BEGIN Includes */
#include "tx_app.h"
/* USER CODE END Includes */

...

/* USER CODE BEGIN App_ThreadX_Init */
tx_setup(memory_ptr);
/* USER CODE END App_ThreadX_Init */
```

### NetX init

```C
#include "nx_app.h"

...

UINT MX_NetXDuo_Init(VOID *memory_ptr) {

    ...

    /* USER CODE BEGIN 0 */
    ret = nx_setup(memory_ptr);
    /* USER CODE END 0 */

    ...

    return ret;
}
```

## Secure

### Main

```C
#include "boot_main.h"
#include "error.h"

boot_main();

error_handler(ERROR_HAL, HAL_ERROR);
```

### Keys

[primary/Secure/Core/Src/aes.c](primary/Secure/Core/Src/aes.c):

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

### NMI

```C
#include "error.h"

...

void NMI_Handler(void) {
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */
    nmi_handler();
    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

    /* USER CODE END NonMaskableInt_IRQn 1 */
}
```
