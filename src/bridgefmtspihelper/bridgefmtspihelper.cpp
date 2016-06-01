#include <unistd.h>

#include <QIODevice>
#include <QByteArray>
#include <QFile>

#include "bridgefmtspihelper.h"

QBridgeFmtSpiHelper::QBridgeFmtSpiHelper()
{
    // default => compatible to old FPGAs with word by word transaction
    m_ui32RAMBlockWordSize = 1;
}

void QBridgeFmtSpiHelper::SetRAMBlockWordSize(quint32 ui32RAMBlockWordSize)
{
    m_ui32RAMBlockWordSize = ui32RAMBlockWordSize;
}

bool QBridgeFmtSpiHelper::BootLCA(QIODevice *pIODevice, const QString &strLCABootFileName)
{
    bool bOK = true;
    QFile fileFpgaBin(strLCABootFileName);
    if(fileFpgaBin.open(QIODevice::ReadOnly))
    {
        qint64 fileSize = fileFpgaBin.size();
        QByteArray data = fileFpgaBin.read(fileSize);
        qint64 sendSize = pIODevice->write(data);
        if(sendSize != fileSize)
        {
            qWarning("FPGA bootfile'%s' was not send completely (send %lli of %lli)!",
                      qPrintable(strLCABootFileName),
                      sendSize, fileSize);
            bOK = false;
        }
        else
        {
            qInfo("FPGA bootfile'%s' was send successfully (%lli bytes send)",
                      qPrintable(strLCABootFileName),
                      fileSize);

        }
        fileFpgaBin.close();
    }
    else
    {
        qWarning("FPGA bootfile'%s' could not be opened!", qPrintable(strLCABootFileName));
        bOK = false;
    }
    return bOK;
}

/* Note: kernel currently supports synchronous I/O only */
bool QBridgeFmtSpiHelper::ExecCommand(QIODevice *pIODevice, BRIDGE_CMDS cmd, QByteArray *pParamData)
{
    bool bOK = true;
    if(!pIODevice->isOpen())
    {
        qWarning("ExecCommand: SPI device not open!");
        bOK = false;
    }
    else
    {
        bool bReadCmd;
        switch(cmd)
        {
        case BRIDGE_CMD_READ_VERSION:
        case BRIDGE_CMD_READ_PCB1:
        case BRIDGE_CMD_READ_PCB2:
        case BRIDGE_CMD_READ_DEVICE:
            bReadCmd = true;
            break;
        default:
            bReadCmd = false;
            break;
        }

        /* Transfer 1: read cmd / write */
        m_SendRawData.clear();
        /* cmd */
        m_SendRawData.append(bReadCmd ? (char)cmd | 0x80: (char)cmd);
        /* cmd param */
        for(int iParam=0; iParam<BRIDGE_SPI_FRAME_LEN-1;iParam++)
        {
            if(pParamData && pParamData->size() > iParam)
                m_SendRawData.append(pParamData->at(iParam));
            else
                m_SendRawData.append((char)0);
        }
        bOK = pIODevice->write(m_SendRawData) == BRIDGE_SPI_FRAME_LEN;
        m_ReceiveRawData.clear();
        if(bOK)
        {
            if(bReadCmd)
            {
                /* Transfer 2: read data */
                m_ReceiveRawData = pIODevice->read(BRIDGE_SPI_FRAME_LEN);
                bOK = m_ReceiveRawData.size() == BRIDGE_SPI_FRAME_LEN;
                if(!bOK)
                    qWarning("Reading command response was not completed!");
            }
        }
        else
            qWarning("Sending command was not completed!");
    }
    return bOK;
}

bool QBridgeFmtSpiHelper::PrepareWriteRam(QIODevice *pIODeviceCtl, const quint32 ui32Address)
{
    m_SendRawData.clear();
    /* cmd */
    m_SendRawData.append(BRIDGE_CMD_SETUP_RAM_ACCESS);
    /* address */
    QByteArray addrArr;
    quint32 ui32AddressWork = ui32Address;
    for(int iAddrByte=0; iAddrByte<BRIDGE_SPI_FRAME_LEN-1; iAddrByte++)
    {
        addrArr.append((char)(ui32AddressWork >> 24));
        ui32AddressWork = (ui32AddressWork << 8);
    }
    /* Write-enable */
    addrArr[0] = addrArr[0] | 0x80;

    /* Transfer write control command */
    m_SendRawData.append(addrArr);
    bool bOK = pIODeviceCtl->write(m_SendRawData) == BRIDGE_SPI_FRAME_LEN;
    if(!bOK)
        qWarning("Sending control command to prepare RAM write was not completed!");
    return bOK;
}

bool QBridgeFmtSpiHelper::WriteRam(QIODevice *pIODeviceData, const TRam16Data &data)
{
    QByteArray send16Block;
    bool bOK = true;
    quint32 ui32CountTransfers = data.count();
    /* 16bit-word transfers */
    for(quint32 ui32Word=0; ui32Word<ui32CountTransfers; ui32Word++)
    {
        quint16 ui16Val = data[ui32Word];
        quint8 ui8Val = ui16Val/256;
        send16Block.append(ui8Val);
        ui8Val = ui16Val & 0xFF;
        send16Block.append(ui8Val);
        // SPI transfer required?
        quint32 ui32WordInBlock = ui32Word % m_ui32RAMBlockWordSize;
        if(ui32WordInBlock==m_ui32RAMBlockWordSize-1 || ui32Word>=ui32CountTransfers-1)
        {
            if(pIODeviceData->write(send16Block) != send16Block.count())
            {
                bOK = false;
                qWarning("Sending data to RAM was not completed!");
                break;
            }
            send16Block.clear();
        }
    }
    return bOK;
}

bool QBridgeFmtSpiHelper::PrepareReadRam(QIODevice *pIODeviceCtl, const quint32 ui32Address)
{
    m_SendRawData.clear();
    /* cmd */
    m_SendRawData.append(BRIDGE_CMD_SETUP_RAM_ACCESS);
    /* address */
    QByteArray addrArr;
    quint32 ui32AddressWork = ui32Address;
    for(int iAddrByte=0; iAddrByte<BRIDGE_SPI_FRAME_LEN-1; iAddrByte++)
    {
        addrArr.append((char)(ui32AddressWork >> 24));
        ui32AddressWork = (ui32AddressWork << 8);
    }

    /* Transfer Write control command */
    m_SendRawData.append(addrArr);
    bool bOK = pIODeviceCtl->write(m_SendRawData) == BRIDGE_SPI_FRAME_LEN;
    if(!bOK)
        qWarning("Sending control command to prepare RAM read was not completed!");
    return bOK;
}

bool QBridgeFmtSpiHelper::ReadRam(QIODevice *pIODeviceData, TRam16Data &data, const quint32 ui32WordCount)
{
    bool bOK = true;

    m_ReceiveRawData.clear();
    QByteArray receive16Block;
    // Read raw data blockwise
    quint32 ui32WordsToRead = ui32WordCount;
    while(ui32WordsToRead > 0)
    {
        quint32 ui32WordsInBlock = qMin(m_ui32RAMBlockWordSize, ui32WordsToRead);
        receive16Block = pIODeviceData->read(2*ui32WordsInBlock);
        if((quint32)receive16Block.size() != 2*ui32WordsInBlock)
        {
            bOK = false;
            qWarning("Reading data to RAM was not completed!");
            break;
        }
        m_ReceiveRawData.append(receive16Block);
        ui32WordsToRead -= ui32WordsInBlock;
    }
    if(bOK)
    {
        // convert to 16bit data
        quint16 ui16Val;
        quint8 ui8Val;
        if((quint32)data.size() != ui32WordCount)
            data.resize(ui32WordCount);
        for(quint32 ui32Word=0; ui32Word<ui32WordCount; ui32Word++)
        {
            // This looks odd but we have
            // * signed char on PC (QByteArray)
            // * endianess??
            ui8Val = m_ReceiveRawData[0 + 2*ui32Word];
            ui16Val = 256 * ui8Val;
            ui8Val = m_ReceiveRawData[1 + 2*ui32Word];
            ui16Val += ui8Val;
            data[ui32Word] = ui16Val;
        }
    }
    return bOK;
}
