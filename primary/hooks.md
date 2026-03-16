# Hooks

## Non-Secure

Almost all code is contained in the Application and Libraries folders, however certain functions must interact with auto-generated code. This page lists all such scenarios so they can be reimplemented if lost due to re-auto-generation.

### Set MAC address && Ethernet DMA Descriptors

`eth.c`

```C
/* USER CODE BEGIN Header */
...
#include "utils.h"
/* USER CODE END Header */

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

`app_netxduo.c`

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
