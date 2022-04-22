/*
    Copyright 2022 Paul Colby

    This file is part of QtPokit.

    QtPokit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QtPokit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QtPokit.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "infocommand.h"

#include <qtpokit/pokitdevice.h>
#include "qtpokit/statusservice.h"
#include <qtpokit/utils.h>
#include <qtpokit/uuids.h>

#include <QCoreApplication>
#include <QDataStream>
#include <QJsonDocument>
#include <QLowEnergyController>
#include <QtEndian>

/*!
 * Construct a new InfoCommand object with \a parent.
 */
InfoCommand::InfoCommand(QObject * const parent) : DeviceCommand(parent), service(nullptr)
{

}

QStringList InfoCommand::requiredOptions() const
{
    return DeviceCommand::requiredOptions() + QStringList{
        QLatin1String("device"),
    };
}

QStringList InfoCommand::supportedOptions() const
{
    return DeviceCommand::supportedOptions();
}

QStringList InfoCommand::processOptions(const QCommandLineParser &parser)
{
    QStringList errors = DeviceCommand::processOptions(parser);
    if (!errors.isEmpty()) {
        return errors;
    }

    return errors;
}

/*!
 * Begins scanning for Pokit devices.
 */
bool InfoCommand::start()
{
    Q_ASSERT(device);
    if (!service) {
        service = device->status();
        Q_ASSERT(service);
        connect(service, &StatusService::serviceDetailsDiscovered,
                this, &InfoCommand::serviceDetailsDiscovered);
    }
    qCDebug(lc).noquote() << tr("Connecting to device...");
    device->controller()->connectToDevice();
    return true;
}

void InfoCommand::serviceDetailsDiscovered()
{
    const QString deviceName = service->deviceName();
    const StatusService::DeviceStatus deviceStatus = service->deviceStatus();
    const QString statusLabel = StatusService::deviceStatusLabel(deviceStatus);
    const float batteryVoltage = service->batteryVoltage();
    const StatusService::DeviceCharacteristics chrs = service->deviceCharacteristics();
    if (chrs.firmwareVersion.isNull()) {
        qCWarning(lc).noquote() << tr("Failed to parse device information");
        QCoreApplication::exit(EXIT_FAILURE);
    }

    switch (format) {
    case OutputFormat::Csv:
        fputs(qPrintable(tr("deviceName,deviceStatus,firmwareVersion,maximumVoltage,maximumCurrent,"
                            "maximumResistance,maximumSamplingRate,samplingBufferSize,"
                            "capabilityMask,macAddress,batteryVoltage\n")), stdout);
        fputs(qPrintable(tr("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11\n")
            .arg(escapeCsvField(deviceName),statusLabel,chrs.firmwareVersion.toString()).arg(chrs.maximumVoltage)
            .arg(chrs.maximumCurrent).arg(chrs.maximumResistance).arg(chrs.maximumSamplingRate)
            .arg(chrs.samplingBufferSize).arg(chrs.capabilityMask).arg(chrs.macAddress.toString())
            .arg(batteryVoltage)), stdout);
        break;
    case OutputFormat::Json:
        fputs(QJsonDocument(QJsonObject{
                { QLatin1String("deviceName"),   deviceName },
                { QLatin1String("deviceStatus"), QJsonObject{
                      { QLatin1String("code"), (quint8)deviceStatus },
                      { QLatin1String("label"), statusLabel },
                }},
                { QLatin1String("firmwareVersion"), QJsonObject{
                      { QLatin1String("major"), chrs.firmwareVersion.majorVersion() },
                      { QLatin1String("minor"), chrs.firmwareVersion.minorVersion() },
                }},
                { QLatin1String("maximumVoltage"),      chrs.maximumVoltage },
                { QLatin1String("maximumCurrent"),      chrs.maximumCurrent },
                { QLatin1String("maximumResistance"),   chrs.maximumResistance },
                { QLatin1String("maximumSamplingRate"), chrs.maximumSamplingRate },
                { QLatin1String("samplingBufferSize"),  chrs.samplingBufferSize },
                { QLatin1String("capabilityMask"),      chrs.capabilityMask },
                { QLatin1String("macAddress"),          chrs.macAddress.toString() },
                { QLatin1String("batteryVoltage"),      batteryVoltage },
            }).toJson(), stdout);
        break;
    case OutputFormat::Text:
        fputs(qPrintable(tr("Device name:           %1\n").arg(deviceName)), stdout);
        fputs(qPrintable(tr("Device status:         %1 (%2)\n").arg(statusLabel).arg((quint8)deviceStatus)), stdout);
        fputs(qPrintable(tr("Firmware version:      %1\n").arg(chrs.firmwareVersion.toString())), stdout);
        fputs(qPrintable(tr("Maximum voltage:       %1\n").arg(chrs.maximumVoltage)), stdout);
        fputs(qPrintable(tr("Maximum current:       %1\n").arg(chrs.maximumCurrent)), stdout);
        fputs(qPrintable(tr("Maximum resistance:    %1\n").arg(chrs.maximumResistance)), stdout);
        fputs(qPrintable(tr("Maximum sampling rate: %1\n").arg(chrs.maximumSamplingRate)), stdout);
        fputs(qPrintable(tr("Sampling buffer size:  %1\n").arg(chrs.samplingBufferSize)), stdout);
        fputs(qPrintable(tr("Capability mask:       %1\n").arg(chrs.capabilityMask)), stdout);
        fputs(qPrintable(tr("MAC address:           %1\n").arg(chrs.macAddress.toString())), stdout);
        fputs(qPrintable(tr("Battery voltage:       %1\n").arg(batteryVoltage)), stdout);
        break;
    }
    QCoreApplication::quit(); // We're all done :)
}
