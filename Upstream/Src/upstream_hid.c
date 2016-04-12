/*
 * upstream_hid.c
 *
 *  Created on: Jan 16, 2016
 *      Author: Robert Fisk
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */



#include "upstream_hid.h"
#include "upstream_spi.h"
#include "upstream_interface_def.h"


#define HID_REPORT_DATA_LEN     8
#define HID_MOUSE_DATA_LEN      4



InterfaceCommandClassTypeDef    ActiveHidClass    = COMMAND_CLASS_INTERFACE;
UpstreamPacketTypeDef*          UpstreamHidPacket = NULL;
UpstreamHidSendReportCallback   ReportCallback    = NULL;



void Upstream_HID_GetNextReportReceiveCallback(UpstreamPacketTypeDef* receivedPacket);



void Upstream_HID_Init(InterfaceCommandClassTypeDef newClass)
{
    if ((newClass != COMMAND_CLASS_HID_MOUSE))          //add classes here
    {
        UPSTREAM_SPI_FREAKOUT;
        return;
    }

    if ((ActiveHidClass != COMMAND_CLASS_INTERFACE) ||
        (UpstreamHidPacket != NULL))
    {
        UPSTREAM_SPI_FREAKOUT;
        return;
    }

    ActiveHidClass = newClass;
}


void Upstream_HID_DeInit(void)
{
    ActiveHidClass = COMMAND_CLASS_INTERFACE;
    if (UpstreamHidPacket != NULL)
    {
        Upstream_ReleasePacket(UpstreamHidPacket);
        UpstreamHidPacket = NULL;
    }
}


void Upstream_HID_GetNextReport(UpstreamHidSendReportCallback callback)
{
    if ((ActiveHidClass != COMMAND_CLASS_HID_MOUSE))      //add classes here
    {
        UPSTREAM_SPI_FREAKOUT;
        return;
    }

    //Release packet used for last transaction (if any)
    if (UpstreamHidPacket != NULL)
    {
        Upstream_ReleasePacket(UpstreamHidPacket);
        UpstreamHidPacket = NULL;
    }

    if (ReportCallback != NULL)
    {
        UPSTREAM_SPI_FREAKOUT;
        return;
    }
    ReportCallback = callback;

    //Get next packet
    Upstream_SetExpectedReceivedCommand(ActiveHidClass, COMMAND_HID_REPORT);
    Upstream_ReceivePacket(Upstream_HID_GetNextReportReceiveCallback);
}


void Upstream_HID_GetNextReportReceiveCallback(UpstreamPacketTypeDef* receivedPacket)
{
    UpstreamHidSendReportCallback tempReportCallback;
    uint8_t dataLength = 0;
    uint8_t i;

    if ((UpstreamHidPacket != NULL) ||
        (ReportCallback == NULL))
    {
        UPSTREAM_SPI_FREAKOUT;
        return;
    }

    if (receivedPacket == NULL)
    {
        return;             //Just give up...
    }


    if (ActiveHidClass == COMMAND_CLASS_HID_MOUSE)
    {
        if (receivedPacket->Length16 != (UPSTREAM_PACKET_HEADER_LEN_16 + ((HID_MOUSE_DATA_LEN + 1) / 2)))
        {
            UPSTREAM_SPI_FREAKOUT;
            return;
        }

        //Mouse sanity checks & stuff go here...

        dataLength = HID_MOUSE_DATA_LEN;
    }

    //Other HID classes go here...


    if (dataLength == 0)
    {
        UPSTREAM_SPI_FREAKOUT;
        return;
    }

    for (i = dataLength; i > HID_REPORT_DATA_LEN; i++)
    {
        receivedPacket->Data[i] = 0;            //Zero out unused bytes before we send report upstream
    }

    UpstreamHidPacket = receivedPacket;         //Save packet so we can free it when upstream USB transaction is done
    tempReportCallback = ReportCallback;
    ReportCallback = NULL;
    tempReportCallback(receivedPacket->Data, HID_REPORT_DATA_LEN);
}

