/*
 * Copyright (c) 2021-2021, Bluetrum Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Change Logs:
 * Data             Version     Notes
 * 2021-11-19       v1.0        First version
 */

#ifndef __DRIVER_SPI_H__
#define __DRIVER_SPI_H__

#ifdef  __cplusplus
extern "C"
{
#endif

#include "driver_common.h"

#define BLUE_SPI_API_VERSION                BLUE_DRIVER_VERSION_MAJOR_MINOR(1, 0)  /* API version */

#define BLUE_SPI_CONTROL_POS                 0
#define BLUE_SPI_CONTROL_MASK               (0xFFUL << BLUE_SPI_CONTROL_POS)

/*----- SPI Control Codes: Mode -----*/
#define BLUE_SPI_MODE_INACTIVE              (0x00UL << BLUE_SPI_CONTROL_POS)     ///< SPI Inactive
#define BLUE_SPI_MODE_MASTER                (0x01UL << BLUE_SPI_CONTROL_POS)     ///< SPI Master (Output on MOSI, Input on MISO); arg = Bus Speed in bps
#define BLUE_SPI_MODE_SLAVE                 (0x02UL << BLUE_SPI_CONTROL_POS)     ///< SPI Slave  (Output on MISO, Input on MOSI)
#define BLUE_SPI_MODE_MASTER_SIMPLEX        (0x03UL << BLUE_SPI_CONTROL_POS)     ///< SPI Master (Output/Input on MOSI); arg = Bus Speed in bps
#define BLUE_SPI_MODE_SLAVE_SIMPLEX         (0x04UL << BLUE_SPI_CONTROL_POS)     ///< SPI Slave  (Output/Input on MISO)

#define BLUE_SPI_SET_MAPPING                (0x20UL << BLUE_SPI_CONTROL_POS)     ///< Set SPI mapping; arg = group (from 1 to 5);
                                                                                 ///< (data_in clk data_out) G1: PA2 PA3 PA4; G2: PA5 PA6 PA7
                                                                                 ///< G3: PB0 PB1 PB2; G4: PE5 PE6 PE7; G%: PF0 PF1 PF2
#define BLUE_SPI_ENABLE                     (0x21UL << BLUE_SPI_CONTROL_POS)

typedef void (*blue_spi_signal_event_t)(uint32_t event);

struct blue_drv_spi
{
    blue_driver_version_t (*get_version)(void);
    int32_t (*init)(blue_spi_signal_event_t cb_event);
    int32_t (*deinit)(void);
    int32_t (*power)(uint32_t state);
    int32_t (*send)(const void *data, uint32_t num);
    int32_t (*recv)(void *data, uint32_t num);
    int32_t (*send_byte)(uint8_t data, uint16_t timeout);
    int32_t (*recv_byte)(uint16_t timeout);
    int32_t (*control)(uint32_t control, uint32_t arg);
};

typedef const struct blue_drv_spi *blue_drv_spi_t;
extern const struct blue_drv_spi blue_spi1;

#ifdef  __cplusplus
}
#endif

#endif /* __DRIVER_SPI_H__ */
