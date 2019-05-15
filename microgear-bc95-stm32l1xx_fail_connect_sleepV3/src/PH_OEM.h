#ifndef PH_OEM_H_
#define PH_OEM_H_

#include "stm32l1xx.h"
#include "stm32l1xx_i2c.h"
#include <stdbool.h>

bool OEM_READ_PH(I2C_TypeDef* i2c, float* raw);
bool OEM_ACTIVE(I2C_TypeDef* i2c);
bool OEM_DEACTIVE(I2C_TypeDef* i2c);

#endif