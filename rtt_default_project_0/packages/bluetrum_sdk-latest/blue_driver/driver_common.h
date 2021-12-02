/*
 * Copyright (c) 2021-2021, Bluetrum Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Change Logs:
 * Data             Version     Notes
 * 2021-11-19       v1.0        First version
 */

#ifndef __DRIVER_COMMON_H__
#define __DRIVER_COMMON_H__

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BLUE_DRIVER_VERSION_MAJOR_MINOR(major, minor)    (((major) << 8) | (minor))

/**
\brief Driver Version
*/
struct blue_driver_version {
  uint16_t api;                         ///< API version
  uint16_t drv;                         ///< Driver version
};

typedef struct blue_driver_version* blue_driver_version_t;

/* General return codes */
#define BLUE_DRIVER_OK                   0 ///< Operation succeeded 
#define BLUE_DRIVER_ERROR               -1 ///< Unspecified error
#define BLUE_DRIVER_ERROR_BUSY          -2 ///< Driver is busy
#define BLUE_DRIVER_ERROR_TIMEOUT       -3 ///< Timeout occurred
#define BLUE_DRIVER_ERROR_UNSUPPORTED   -4 ///< Operation not supported
#define BLUE_DRIVER_ERROR_PARAMETER     -5 ///< Parameter error
#define BLUE_DRIVER_ERROR_SPECIFIC      -6 ///< Start of driver specific errors 

#ifdef  __cplusplus
}
#endif

#endif
