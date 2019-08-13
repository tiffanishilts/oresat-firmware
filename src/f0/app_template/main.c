/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/* ChibiOS header files */
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

/* Project header files */
#include "oresat.h"
#include "thread1.h"

/* Serial configuration */
static SerialConfig ser_cfg =
{
    115200,                                 /* Baud rate    */
    0,                                      /* CR1 Config   */
    0,                                      /* CR2 Config   */
    0,                                      /* CR3 Config   */
};

/* ADC Group Configuration */
static const ADCConversionGroup adcgrpcfg =
{
    TRUE,                                   /* Circular Buffer Mode */
    1,                                      /* Number of Channels   */
    NULL,                                   /* Completion Callback  */
    NULL,                                   /* Error Callback       */
    ADC_CFGR1_CONT | ADC_CFGR1_RES_12BIT,   /* CFGR1                */
    ADC_TR(0, 0),                           /* TR                   */
    ADC_SMPR_SMP_28P5,                      /* SMPR                 */
    ADC_CHSELR_CHSEL16                      /* CHSELR               */
};

static void app_init(void)
{
    /* App initialization */
    reg_worker("Example thread", waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    /* Start up debug output */
    sdStart(&SD2, &ser_cfg);
    adcStart(&ADCD1, NULL);
    adcSTM32SetCCR(ADC_CCR_VBATEN | ADC_CCR_TSEN | ADC_CCR_VREFEN);
    /*adcStartConversion(&ADCD1, &adcgrpcfg, OD_temperature, ODL_temperature_arrayLength);*/
    /*adcStopConversion(&ADCD1);*/
}

int main(void)
{
    // Initialize and start
    oresat_init(ORESAT_DEFAULT_ID, ORESAT_DEFAULT_BITRATE);
    app_init();
    oresat_start(&CAND1);
    return 0;
}
