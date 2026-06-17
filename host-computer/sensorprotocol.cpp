#include "sensorprotocol.h"

// ─────────────────────────────────────────────────
// 帧构建：AA AA CMD LEN DATA... XOR 55
// ─────────────────────────────────────────────────
QByteArray buildFrame(quint8 cmd, const QByteArray &data)
{
    quint8 xorVal = 0;
    for (quint8 b : data) xorVal ^= b;

    QByteArray frame;
    frame.append(static_cast<char>(PC_TO_DEV_HEADER1));
    frame.append(static_cast<char>(PC_TO_DEV_HEADER2));
    frame.append(static_cast<char>(cmd));
    frame.append(static_cast<char>(data.size()));
    frame.append(data);
    frame.append(static_cast<char>(xorVal));
    frame.append(static_cast<char>(PC_TO_DEV_TAIL));
    return frame;
}

// ─────────────────────────────────────────────────
// 状态机解析器
// ─────────────────────────────────────────────────
FrameParser::FrameParser(QObject *parent) : QObject(parent) {}

void FrameParser::feed(const QByteArray &bytes)
{
    for (quint8 byte : bytes) {
        switch (m_state) {
        case HEADER1:
            if (byte == DEV_TO_PC_HEADER1) m_state = HEADER2;
            break;
        case HEADER2:
            m_state = (byte == DEV_TO_PC_HEADER2) ? CMD : HEADER1;
            break;
        case CMD:
            m_cmd   = byte;
            m_state = LEN;
            break;
        case LEN:
            m_dataLen  = byte;
            m_checksum = 0;
            m_buf.clear();
            if (byte == 0)       m_state = CHECKSUM;
            else if (byte <= 64) m_state = DATA;
            else                 m_state = HEADER1;
            break;
        case DATA:
            m_buf.append(static_cast<char>(byte));
            m_checksum ^= byte;
            if (m_buf.size() >= m_dataLen) m_state = CHECKSUM;
            break;
        case CHECKSUM:
            m_state = (byte == m_checksum) ? TAIL : HEADER1;
            break;
        case TAIL:
            if (byte == DEV_TO_PC_TAIL)
                emit frameReady({ m_cmd, m_buf });
            m_state = HEADER1;
            break;
        }
    }
}
