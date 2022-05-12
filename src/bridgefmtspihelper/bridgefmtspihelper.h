#ifndef BRIDGEFMTSPIHELPER_H
#define BRIDGEFMTSPIHELPER_H

#include <bridgefmtspihelper_export.h>
#include <QVector>
#include <QByteArray>
#include <QString>
#include <QIODevice>

#define BRIDGE_SPI_FRAME_LEN 5

enum BRIDGE_CMDS
{
    BRIDGE_CMD_READ_VERSION         = 0x00,
    BRIDGE_CMD_READ_PCB1            = 0x01,
    BRIDGE_CMD_READ_PCB2            = 0x02,
    BRIDGE_CMD_READ_DEVICE          = 0x03,
    BRIDGE_CMD_SETUP_RAM_ACCESS     = 0x10,
};

typedef QVector<qint16> TRam16Data;

class BRIDGEFMTSPIHELPER_EXPORT QBridgeFmtSpiHelper
{
public:
    QBridgeFmtSpiHelper();
    void SetRAMBlockWordSize(quint32 ui32RAMBlockWordSize);

    bool BootLCA(QIODevice *pIODevice, const QString& strLCABootFileName);
    bool ExecCommand(QIODevice *pIODevice, enum BRIDGE_CMDS cmd, QByteArray *pParamData = nullptr);

    bool PrepareWriteRam(QIODevice *pIODeviceCtl, const quint32 ui32Address);
    bool WriteRam(QIODevice *pIODeviceData, const TRam16Data& data);

    bool PrepareReadRam(QIODevice *pIODeviceCtl, const quint32 ui32Address);
    bool ReadRam(QIODevice *pIODeviceData, TRam16Data& data, const quint32 ui32WordCount);

    const QByteArray& GetSendRawData() { return m_SendRawData; }
    const QByteArray& GetReceiveRawData() { return m_ReceiveRawData; }
protected:
    QByteArray m_SendRawData;
    QByteArray m_ReceiveRawData;
    quint32 m_ui32RAMBlockWordSize;
};

#endif // BRIDGEFMTSPIHELPER_H
