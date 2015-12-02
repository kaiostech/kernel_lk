/*
 * Copyright (c) 2015 Eric Holland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <platform/gpio.h>
#include <target/gpioconfig.h>
#include <target/e-ink.h>

static I2C_HandleTypeDef i2c_handle;

void eink_init_early(void)
{
    gpio_config( GPIO_EINK_ENA, GPIO_OUTPUT );
    gpio_config( GPIO_EINK_POWERON, GPIO_OUTPUT );
    gpio_config( GPIO_EINK_SCL,GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF4_I2C1));
    gpio_config( GPIO_EINK_SDA,GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF4_I2C1));

   // i2c_handle.Init.Timing             =
   // i2c_handle.Init.OwnAddress1        =
    i2c_handle.Init.AddressingMode      = I2C_ADDRESSINGMODE_7BIT;
    i2c_handle.Init.DualAddressMode    = I2C_DUALADDRESS_DISABLE;
    i2c_handle.Init.OwnAddress2        = 0;
    i2c_handle.Init.OwnAddress2Masks   = I2C_OA2_NOMASK;
    i2c_handle.Init.GeneralCallMode    = I2C_GENERALCALL_DISABLE;
    i2c_handle.Init.NoStretchMode      = I2C_NOSTRETCH_DISABLE;

    i2c_handle.Instance         = I2C1;


    gpio_set(GPIO_EINK_ENA, GPIO_PIN_RESET);
    gpio_set(GPIO_EINK_POWERON, GPIO_PIN_RESET);
}

void eink_powerup(void)
{
    gpio_set(GPIO_EINK_ENA, GPIO_PIN_SET);
    gpio_set(GPIO_EINK_POWERON, GPIO_PIN_SET);
}
