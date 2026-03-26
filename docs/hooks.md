# Hooks

## Non-Secure

Almost all code is contained in the Application and Libraries folders, however certain functions must interact with auto-generated code. This page lists all such scenarios so they can be reimplemented if lost due to re-auto-generation.

### Expose functions in main

`main.c`

```C
/* USER CODE BEGIN Includes */
#include "app.h"
/* USER CODE END Includes */

...

/* USER CODE BEGIN 0 */
void mpu_config() {
    MPU_Config();
}
/* USER CODE END 0 */
```

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


## Secure

### Expose functions in main and set NS vector table address
`main.c`

```C

/* USER CODE BEGIN Includes */
#include "error.h"
#include "app.h"
/* USER CODE END Includes */

...

/* USER CODE BEGIN VTOR_TABLE */
#define VTOR_TABLE_NS_START_ADDR ((uint32_t) &__FLASH_NSC_END__)
/* USER CODE END VTOR_TABLE */

...

/* USER CODE BEGIN PV */
extern uint32_t __FLASH_NSC_END__;
/* USER CODE END PV */

...

/* USER CODE BEGIN 0 */
void nonsecure_init() {
    NonSecure_Init();
}
void system_clock_config() {
    SystemClock_Config();
}
void periph_common_clock_config() {
    PeriphCommonClock_Config();
}
void mpu_config() {
    MPU_Config();
}
/* USER CODE END 0 */

...

  /* USER CODE BEGIN Error_Handler_Debug */
  error_handler(BL_HAL_ERROR);
  /* USER CODE END Error_Handler_Debug */

```
