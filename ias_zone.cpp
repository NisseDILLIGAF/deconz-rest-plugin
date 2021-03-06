/*
 * Copyright (c) 2017 dresden elektronik ingenieurtechnik gmbh.
 * All rights reserved.
 *
 * The software in this package is published under the terms of the BSD
 * style license a copy of which has been included with this distribution in
 * the LICENSE.txt file.
 *
 */

#include <QString>
#include <QVariantMap>
#include "de_web_plugin.h"
#include "de_web_plugin_private.h"
#include "json.h"

// server send
#define CMD_STATUS_CHANGE_NOTIFICATION 0x00
#define CMD_ZONE_ENROLL_REQUEST 0x01
// server receive
#define CMD_ZONE_ENROLL_RESPONSE 0x00

// Zone status flags
#define STATUS_ALARM1         0x0001
#define STATUS_ALARM2         0x0002
#define STATUS_TAMPER         0x0004
#define STATUS_BATTERY        0x0008
#define STATUS_SUPERVISION    0x0010
#define STATUS_RESTORE_REP    0x0020
#define STATUS_TROUBLE        0x0040
#define STATUS_AC_MAINS       0x0080
#define STATUS_TEST           0x0100
#define STATUS_BATTERY_DEFECT 0x0200

/*! Handle packets related to the ZCL IAS Zone cluster.
    \param ind the APS level data indication containing the ZCL packet
    \param zclFrame the actual ZCL frame which holds the IAS zone server command
 */
void DeRestPluginPrivate::handleIasZoneClusterIndication(const deCONZ::ApsDataIndication &ind, deCONZ::ZclFrame &zclFrame)
{
    Q_UNUSED(ind);

    QDataStream stream(zclFrame.payload());
    stream.setByteOrder(QDataStream::LittleEndian);

    if (!(zclFrame.frameControl() & deCONZ::ZclFCDirectionServerToClient))
    {
        return;
    }

    if (zclFrame.commandId() == CMD_STATUS_CHANGE_NOTIFICATION)
    {
        quint16 zoneStatus;
        quint8 extendedStatus;
        quint8 zoneId;
        quint16 delay;

        stream >> zoneStatus;
        stream >> extendedStatus; // reserved, set to 0
        stream >> zoneId;
        stream >> delay;

        DBG_Printf(DBG_ZCL, "IAS Zone Status Change, status: 0x%04X, zoneId: %u, delay: %u\n", zoneStatus, zoneId, delay);

        Sensor *sensor = getSensorNodeForAddressAndEndpoint(ind.srcAddress(), ind.srcEndpoint());
        ResourceItem *item = sensor ? sensor->item(RStatePresence) : 0;

        if (sensor && item)
        {
            bool presence = zoneStatus & (STATUS_ALARM1 | STATUS_ALARM2);
            item->setValue(presence);
            sensor->updateStateTimestamp();

            item = sensor->item(RConfigReachable);
            if (item && !item->toBool())
            {
                item->setValue(true);
                Event e(RSensors, RConfigReachable, sensor->id());
                enqueueEvent(e);
            }

            updateSensorEtag(sensor);

            Event e(RSensors, RStatePresence, sensor->id());
            enqueueEvent(e);
        }

    }
    else if (zclFrame.commandId() == CMD_ZONE_ENROLL_REQUEST)
    {
        quint16 zoneType;
        quint16 manufacturer;

        stream >> zoneType;
        stream >> manufacturer;

        DBG_Printf(DBG_ZCL, "IAS Zone Enroll Request, zone type: 0x%04X, manufacturer: 0x%04X\n", zoneType, manufacturer);

        sendIasZoneEnrollResponse(ind, zclFrame);
    }
}

/*! Sends IAS Zone enroll response to IAS Zone server.
    \param ind the APS level data indication containing the ZCL packet
    \param zclFrame the actual ZCL frame which holds the IAS Zone enroll request
 */
void DeRestPluginPrivate::sendIasZoneEnrollResponse(const deCONZ::ApsDataIndication &ind, deCONZ::ZclFrame &zclFrame)
{
    deCONZ::ApsDataRequest req;
    deCONZ::ZclFrame outZclFrame;

    req.setProfileId(ind.profileId());
    req.setClusterId(ind.clusterId());
    req.setDstAddressMode(ind.srcAddressMode());
    req.dstAddress() = ind.srcAddress();
    req.setDstEndpoint(ind.srcEndpoint());
    req.setSrcEndpoint(endpoint());

    outZclFrame.setSequenceNumber(zclFrame.sequenceNumber());
    outZclFrame.setCommandId(CMD_ZONE_ENROLL_RESPONSE);

    outZclFrame.setFrameControl(deCONZ::ZclFCClusterCommand |
                             deCONZ::ZclFCDirectionClientToServer |
                             deCONZ::ZclFCDisableDefaultResponse);

    { // payload
        QDataStream stream(&outZclFrame.payload(), QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);

        quint8 code = 0x00; // success
        quint8 zoneId = 100;

        stream << code;
        stream << zoneId;
    }

    { // ZCL frame
        QDataStream stream(&req.asdu(), QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        outZclFrame.writeToStream(stream);
    }

    if (apsCtrl && apsCtrl->apsdeDataRequest(req) != deCONZ::Success)
    {
        DBG_Printf(DBG_INFO, "IAS Zone failed to send enroll reponse\n");
    }
}
