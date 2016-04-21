#include <unistd.h>

#include <QIODevice>
#include <QByteArray>
#include <QFile>

#include "bridgefmtspihelper.h"

BridgeFmtSpiHelper::BridgeFmtSpiHelper()
{
}

bool BridgeFmtSpiHelper::BootLCA(QIODevice *pIODevice, const QString &strLCABootFileName)
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

#define BRIDGE_SPI_FRAME_LEN 5

/* Note: kernel currently supports synchronous I/O only */
bool BridgeFmtSpiHelper::ExecCommand(QIODevice *pIODevice, BRIDGE_CMDS cmd, QByteArray *pParamData)
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

bool BridgeFmtSpiHelper::PrepareWriteRam(QIODevice *pIODeviceCtl, const quint32 ui32Address)
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

bool BridgeFmtSpiHelper::WriteRam(QIODevice *pIODeviceData, const TRam16Data &data)
{
    QByteArray send16Word;
    bool bOK = true;
    /* n-16bit transfers */
    for(int iWord=0; iWord<data.count(); iWord++)
    {
        send16Word.clear();
        qint16 i16Val = data[iWord];
        send16Word.append((char)(i16Val>>8));
        send16Word.append((char)(i16Val & 0xFF));
        if(pIODeviceData->write(send16Word) != send16Word.count())
        {
            bOK = false;
            qWarning("Sending data to RAM was not completed!");
            break;
        }
    }
    return bOK;
}

bool BridgeFmtSpiHelper::PrepareReadRam(QIODevice *pIODeviceCtl, const quint32 ui32Address)
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

bool BridgeFmtSpiHelper::ReadRam(QIODevice *pIODeviceData, TRam16Data &data, const quint32 ui32WordCount)
{
    bool bOK = true;
    m_ReceiveRawData.clear();
    data.resize(ui32WordCount);
    QByteArray receive16Word;
    qint16 i16Val;
    /* n-16bit transfers */
    for(quint32 ui32Word=0; ui32Word<ui32WordCount; ui32Word++)
    {
        receive16Word = pIODeviceData->read(2);
        if(receive16Word.size() == 2)
        {
            m_ReceiveRawData.append(receive16Word);
            i16Val = (receive16Word[0] << 8 ) + receive16Word[1];
            data[ui32Word] = i16Val;
        }
        else
        {
            bOK = false;
            qWarning("Reading data to RAM was not completed!");
            break;
        }
    }
    return bOK;
}
