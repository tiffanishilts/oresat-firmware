/*
 * CAN module object for generic microcontroller.
 *
 * This file is a template for other microcontrollers.
 *
 * @file        CO_driver.c
 * @ingroup     CO_driver
 * @author      Janez Paternoster
 * @copyright   2004 - 2015 Janez Paternoster
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * CANopenNode is free and open source software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Following clarification and special exception to the GNU General Public
 * License is included to the distribution terms of CANopenNode:
 *
 * Linking this library statically or dynamically with other modules is
 * making a combined work based on this library. Thus, the terms and
 * conditions of the GNU General Public License cover the whole combination.
 *
 * As a special exception, the copyright holders of this library give
 * you permission to link this library with independent modules to
 * produce an executable, regardless of the license terms of these
 * independent modules, and to copy and distribute the resulting
 * executable under terms of your choice, provided that you also meet,
 * for each linked independent module, the terms and conditions of the
 * license of that module. An independent module is a module which is
 * not derived from or based on this library. If you modify this
 * library, you may extend this exception to your version of the
 * library, but you are not obliged to do so. If you do not wish
 * to do so, delete this exception statement from your version.
 */


#include "CO_driver.h"
#include "CO_Emergency.h"
#include "can_hw.h"

CANConfig cancfg = {
    // MCR (Master Control Register)
    CAN_MCR_ABOM      |     //Automatic Bus-Off Management
    CAN_MCR_AWUM      |     //Automatic Wakeup Mode
    CAN_MCR_TXFP      ,     //Transmit FIFO Priority
    // BTR (Bit Timing Register)
    //CAN_BTR_LBKM     |     //Loopback Mode (Debug)
    CAN_BTR(1000)           //Default to 1000Mbps
};

/******************************************************************************/
void CO_CANsetConfigurationMode(int32_t CANbaseAddress){
    /* Put CAN module in configuration mode */
    canStop((CANDriver *)CANbaseAddress);
}


/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule){
    /* Put CAN module in normal mode */
    canSTM32SetFilters((CANDriver *)CANmodule->CANbaseAddress, 0xE, CANmodule->useCANrxFilters, CANmodule->canFilters);
    canStart((CANDriver *)CANmodule->CANbaseAddress, &cancfg);
    CANmodule->CANnormal = true;
}


/******************************************************************************/
CO_ReturnError_t CO_CANmodule_init(
        CO_CANmodule_t         *CANmodule,
        int32_t                 CANbaseAddress,
        CO_CANrx_t              rxArray[],
        uint16_t                rxSize,
        CO_CANtx_t              txArray[],
        uint16_t                txSize,
        uint16_t                CANbitRate)
{
    uint16_t i;

    /* Verify arguments */
    if(CANmodule==NULL || rxArray==NULL || txArray==NULL){
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Configure object variables */
    CANmodule->CANbaseAddress = CANbaseAddress;
    CANmodule->rxArray = rxArray;
    CANmodule->rxSize = rxSize;
    CANmodule->txArray = txArray;
    CANmodule->txSize = txSize;
    CANmodule->CANnormal = false;
    CANmodule->useCANrxFilters = (rxSize <= STM32_CAN_MAX_FILTERS ? rxSize : 0);
    CANmodule->bufferInhibitFlag = false;
    CANmodule->firstCANtxMessage = true;
    CANmodule->CANtxCount = 0U;
    CANmodule->errOld = 0U;
    CANmodule->em = NULL;

    for(i=0U; i<rxSize; i++){
        rxArray[i].ident = 0U;
        rxArray[i].mask = 0xFFFFU;
        rxArray[i].object = NULL;
        rxArray[i].pFunct = NULL;
    }
    for(i=0U; i<txSize; i++){
        txArray[i].bufferFull = false;
    }


    /* TODO: Check if we need this: Configure CAN module registers */


    /* Configure CAN timing */
    cancfg.btr = CAN_BTR(CANbitRate);

    /* Configure CAN module hardware filters */
    if(CANmodule->useCANrxFilters){
        /* CAN module filters are used, they will be configured with */
        /* CO_CANrxBufferInit() functions, called by separate CANopen */
        /* init functions. */
        /* Configure all masks so that received message must match filter */
        for (i = 0U; i < rxSize; i++) {
            CANmodule->canFilters[i].filter = i;
            CANmodule->canFilters[i].mode = 0;                  /* Mask Mode */
            CANmodule->canFilters[i].scale = 1;                 /* 32bit scale (easy but inefficient) */
            CANmodule->canFilters[i].assignment = 0;            /* Assign FIFO0 */
            CANmodule->canFilters[i].register1 = 0;             /* Clear out the ID */
            CANmodule->canFilters[i].register2 = 0xFFFFFFFFU;   /* Initialize masks */
        }
    }

    /* TODO: Check if we need this: Configure CAN interrupt registers */


    return CO_ERROR_NO;
}


/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule){
    /* turn off the module */
    canStop((CANDriver *)CANmodule->CANbaseAddress);
}


/******************************************************************************/
uint16_t CO_CANrxMsg_readIdent(const CO_CANrxMsg_t *rxMsg){
    return (uint16_t) rxMsg->SID;
}


/******************************************************************************/
CO_ReturnError_t CO_CANrxBufferInit(
        CO_CANmodule_t         *CANmodule,
        uint16_t                index,
        uint16_t                ident,
        uint16_t                mask,
        bool_t                  rtr,
        void                   *object,
        void                  (*pFunct)(void *object, const CO_CANrxMsg_t *message))
{
    CO_ReturnError_t ret = CO_ERROR_NO;

    if((CANmodule!=NULL) && (object!=NULL) && (pFunct!=NULL) && (index < CANmodule->rxSize)){
        /* buffer, which will be configured */
        CO_CANrx_t *buffer = &CANmodule->rxArray[index];

        /* Configure object variables */
        buffer->object = object;
        buffer->pFunct = pFunct;

        /* CAN identifier and CAN mask, bit aligned with CAN module. Different on different microcontrollers. */
        buffer->ident = ident & 0x07FFU;
        if(rtr){
            buffer->ident |= 0x0800U;
        }
        buffer->mask = (mask & 0x07FFU) | 0x0800U;

        /* Set CAN hardware module filter and mask. */
        if(CANmodule->useCANrxFilters){
            flt_reg_t filter;
            filter.raw = 0;
            filter.scale32.STID = ident;
            filter.scale32.RTR = rtr;
            CANmodule->canFilters[index].register1 = filter.raw;
        }
    }
    else{
        ret = CO_ERROR_ILLEGAL_ARGUMENT;
    }

    return ret;
}


/******************************************************************************/
CO_CANtx_t *CO_CANtxBufferInit(
        CO_CANmodule_t         *CANmodule,
        uint16_t                index,
        uint16_t                ident,
        bool_t                  rtr,
        uint8_t                 noOfBytes,
        bool_t                  syncFlag)
{
    CO_CANtx_t *buffer = NULL;

    if((CANmodule != NULL) && (index < CANmodule->txSize)){
        /* get specific buffer */
        buffer = &CANmodule->txArray[index];

        /* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer.
         * Microcontroller specific. */
        buffer->SID = ident;
        buffer->DLC = noOfBytes;
        buffer->RTR = rtr;

        buffer->bufferFull = false;
        buffer->syncFlag = syncFlag;
    }

    return buffer;
}


/******************************************************************************/
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer){
    CO_ReturnError_t err = CO_ERROR_NO;

    /* Verify overflow */
    if(buffer->bufferFull){
        if(!CANmodule->firstCANtxMessage){
            /* Don't set error, if bootup message is still on buffers */
            CO_errorReport((CO_EM_t*)CANmodule->em, CO_EM_CAN_TX_OVERFLOW, CO_EMC_CAN_OVERRUN, buffer->SID);
        }
        err = CO_ERROR_TX_OVERFLOW;
    }

    CO_LOCK_CAN_SEND();
    /* If CAN TX buffer is free, attempt to send it */
    if(CANmodule->CANtxCount == 0 && \
            !canTryTransmitI((CANDriver *)CANmodule->CANbaseAddress, CAN_ANY_MAILBOX, &buffer->txFrame)){
        CANmodule->bufferInhibitFlag = buffer->syncFlag;
    }
    /* If no buffer is free, message will be sent by interrupt */
    else{
        buffer->bufferFull = true;
        CANmodule->CANtxCount++;
    }
    CO_UNLOCK_CAN_SEND();

    return err;
}


/******************************************************************************/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule){
    uint32_t tpdoDeleted = 0U;

    CO_LOCK_CAN_SEND();
    /* Abort message from CAN module, if there is synchronous TPDO.
     * Take special care with this functionality. */
    /* TODO: Figure out how to distinguish synchronous TPDOs in TX mailboxes */
    if(/*messageIsOnCanBuffer && */CANmodule->bufferInhibitFlag){
        /* clear TXREQ */
        CANmodule->bufferInhibitFlag = false;
        tpdoDeleted = 1U;
    }
    /* delete also pending synchronous TPDOs in TX buffers */
    if(CANmodule->CANtxCount != 0U){
        uint16_t i;
        CO_CANtx_t *buffer = &CANmodule->txArray[0];
        for(i = CANmodule->txSize; i > 0U; i--){
            if(buffer->bufferFull){
                if(buffer->syncFlag){
                    buffer->bufferFull = false;
                    CANmodule->CANtxCount--;
                    tpdoDeleted = 2U;
                }
            }
            buffer++;
        }
    }
    CO_UNLOCK_CAN_SEND();


    if(tpdoDeleted != 0U){
        CO_errorReport((CO_EM_t*)CANmodule->em, CO_EM_TPDO_OUTSIDE_WINDOW, CO_EMC_COMMUNICATION, tpdoDeleted);
    }
}


/******************************************************************************/
void CO_CANverifyErrors(CO_CANmodule_t *CANmodule){
    uint16_t rxErrors, txErrors, overflow;
    CO_EM_t* em = (CO_EM_t*)CANmodule->em;
    uint32_t err;

    /* TODO: Check this whole thing after figuring out error handling */
    /* get error counters from module. Id possible, function may use different way to
     * determine errors. */
    rxErrors = CANmodule->txSize;
    txErrors = CANmodule->txSize;
    overflow = CANmodule->txSize;

    err = ((uint32_t)txErrors << 16) | ((uint32_t)rxErrors << 8) | overflow;

    if(CANmodule->errOld != err){
        CANmodule->errOld = err;

        if(txErrors >= 256U){                               /* bus off */
            CO_errorReport(em, CO_EM_CAN_TX_BUS_OFF, CO_EMC_BUS_OFF_RECOVERED, err);
        }
        else{                                               /* not bus off */
            CO_errorReset(em, CO_EM_CAN_TX_BUS_OFF, err);

            if((rxErrors >= 96U) || (txErrors >= 96U)){     /* bus warning */
                CO_errorReport(em, CO_EM_CAN_BUS_WARNING, CO_EMC_NO_ERROR, err);
            }

            if(rxErrors >= 128U){                           /* RX bus passive */
                CO_errorReport(em, CO_EM_CAN_RX_BUS_PASSIVE, CO_EMC_CAN_PASSIVE, err);
            }
            else{
                CO_errorReset(em, CO_EM_CAN_RX_BUS_PASSIVE, err);
            }

            if(txErrors >= 128U){                           /* TX bus passive */
                if(!CANmodule->firstCANtxMessage){
                    CO_errorReport(em, CO_EM_CAN_TX_BUS_PASSIVE, CO_EMC_CAN_PASSIVE, err);
                }
            }
            else{
                bool_t isError = CO_isError(em, CO_EM_CAN_TX_BUS_PASSIVE);
                if(isError){
                    CO_errorReset(em, CO_EM_CAN_TX_BUS_PASSIVE, err);
                    CO_errorReset(em, CO_EM_CAN_TX_OVERFLOW, err);
                }
            }

            if((rxErrors < 96U) && (txErrors < 96U)){       /* no error */
                CO_errorReset(em, CO_EM_CAN_BUS_WARNING, err);
            }
        }

        if(overflow != 0U){                                 /* CAN RX bus overflow */
            CO_errorReport(em, CO_EM_CAN_RXB_OVERFLOW, CO_EMC_CAN_OVERRUN, err);
        }
    }
}


/******************************************************************************/
void CO_CANinterrupt(CO_CANmodule_t *CANmodule){

    /* receive interrupt */
    if(1){
        CO_CANrxMsg_t *rcvMsg;      /* pointer to received message in CAN module */
        uint16_t index;             /* index of received message */
        uint32_t rcvMsgIdent;       /* identifier of the received message */
        CO_CANrx_t *buffer = NULL;  /* receive message buffer from CO_CANmodule_t object. */
        bool_t msgMatched = false;

        rcvMsg = 0; /* get message from module here */
        rcvMsgIdent = rcvMsg->SID;
        if(CANmodule->useCANrxFilters){
            /* CAN module filters are used. Message with known 11-bit identifier has */
            /* been received */
            index = 0;  /* get index of the received message here. Or something similar */
            if(index < CANmodule->rxSize){
                buffer = &CANmodule->rxArray[index];
                /* verify also RTR */
                if(((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U){
                    msgMatched = true;
                }
            }
        }
        else{
            /* CAN module filters are not used, message with any standard 11-bit identifier */
            /* has been received. Search rxArray form CANmodule for the same CAN-ID. */
            buffer = &CANmodule->rxArray[0];
            for(index = CANmodule->rxSize; index > 0U; index--){
                if(((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U){
                    msgMatched = true;
                    break;
                }
                buffer++;
            }
        }

        /* Call specific function, which will process the message */
        if(msgMatched && (buffer != NULL) && (buffer->pFunct != NULL)){
            buffer->pFunct(buffer->object, rcvMsg);
        }

        /* Clear interrupt flag */
    }


    /* transmit interrupt */
    else if(0){
        /* Clear interrupt flag */

        /* First CAN message (bootup) was sent successfully */
        CANmodule->firstCANtxMessage = false;
        /* clear flag from previous message */
        CANmodule->bufferInhibitFlag = false;
        /* Are there any new messages waiting to be send */
        if(CANmodule->CANtxCount > 0U){
            uint16_t i;             /* index of transmitting message */

            /* first buffer */
            CO_CANtx_t *buffer = &CANmodule->txArray[0];
            /* search through whole array of pointers to transmit message buffers. */
            for(i = CANmodule->txSize; i > 0U; i--){
                /* if message buffer is full, send it. */
                if(buffer->bufferFull){
                    buffer->bufferFull = false;
                    CANmodule->CANtxCount--;

                    /* Copy message to CAN buffer */
                    CANmodule->bufferInhibitFlag = buffer->syncFlag;
                    /* canSend... */
                    break;                      /* exit for loop */
                }
                buffer++;
            }/* end of for loop */

            /* Clear counter if no more messages */
            if(i == 0U){
                CANmodule->CANtxCount = 0U;
            }
        }
    }
    else{
        /* some other interrupt reason */
    }
}
