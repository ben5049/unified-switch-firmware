/*
 * common.c
 *
 *  Created on: Mar 24, 2026
 *      Author: bens1
 */

#include "app.h"
#include "bootloader_config.h"
#include "hal.h"
#include "usart.h"

#include "bootloader.h"
#include "log_tools.h"


void HAL_HASH_ErrorCallback(HASH_HandleTypeDef *hhash) {

    switch (HAL_HASH_GetError(hhash)) {
        case (HAL_HASH_ERROR_NONE): {
            LOG_ERROR("HAL_HASH_ERROR_NONE\n");
            break;
        }
        case (HAL_HASH_ERROR_BUSY): {
            LOG_ERROR("HAL_HASH_ERROR_BUSY\n");
            break;
        }
        case (HAL_HASH_ERROR_DMA): {
            switch (HAL_DMA_GetError(hhash->hdmain)) {
                case (HAL_DMA_ERROR_NONE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_NONE\n");
                    break;
                }
                case (HAL_DMA_ERROR_DTE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_DTE\n");
                    break;
                }
                case (HAL_DMA_ERROR_ULE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_ULE\n");
                    break;
                }
                case (HAL_DMA_ERROR_USE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_USE\n");
                    break;
                }
                case (HAL_DMA_ERROR_TO): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_TO\n");
                    break;
                }
                case (HAL_DMA_ERROR_TIMEOUT): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_TIMEOUT\n");
                    break;
                }
                case (HAL_DMA_ERROR_NO_XFER): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_NO_XFER\n");
                    break;
                }
                case (HAL_DMA_ERROR_BUSY): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_BUSY\n");
                    break;
                }
                case (HAL_DMA_ERROR_INVALID_CALLBACK): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_INVALID_CALLBACK\n");
                    break;
                }
                case (HAL_DMA_ERROR_NOT_SUPPORTED): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_NOT_SUPPORTED\n");
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
        case (HAL_HASH_ERROR_TIMEOUT): {
            LOG_ERROR("HAL_HASH_ERROR_TIMEOUT\n");
            break;
        }
#if (USE_HAL_HASH_REGISTER_CALLBACKS == 1U)
        case (HAL_HASH_ERROR_INVALID_CALLBACK): {
            LOG_ERROR("HAL_HASH_ERROR_INVALID_CALLBACK\n");
            break;
        }
#endif
        default:
            break;
    }
    error_handler(BL_HAL_ERROR);
}


void hard_fault_handler() {

    HAL_GPIO_WritePin(STAT_GPIO_Port, STAT_Pin, GPIO_PIN_SET);

#if DEBUG
    HAL_GPIO_WritePin(PHY_CLK_EN_GPIO_Port, PHY_CLK_EN_Pin, GPIO_PIN_RESET); /* Stop the board from cooking itself */
#endif

    while (1);
}
