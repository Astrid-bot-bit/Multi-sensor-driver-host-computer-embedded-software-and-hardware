#pragma once
#include <QObject>
#include <QByteArray>

// ─────────────────────────────────────────────────
// 协议常量
// ─────────────────────────────────────────────────
static constexpr quint8 PC_TO_DEV_HEADER1 = 0xAA;
static constexpr quint8 PC_TO_DEV_HEADER2 = 0xAA;
static constexpr quint8 PC_TO_DEV_TAIL    = 0x55;

static constexpr quint8 DEV_TO_PC_HEADER1 = 0x55;
static constexpr quint8 DEV_TO_PC_HEADER2 = 0x55;
static constexpr quint8 DEV_TO_PC_TAIL    = 0xAA;

static constexpr quint8 CMD_GET_PRODUCT_TYPE = 0x01;
static constexpr quint8 CMD_WRITE_REGISTER   = 0x05;
static constexpr quint8 CMD_READ_REGISTER    = 0x06;
static constexpr quint8 CMD_READ_PRESS_TEMP  = 0x07;
static constexpr quint8 CMD_READ_RAW_DATA    = 0x08;
static constexpr quint8 CMD_SET_OVERSAMPLING = 0x0B;

// ─────────────────────────────────────────────────
// 一帧解析完成后携带的数据
// ─────────────────────────────────────────────────
struct SensorFrame {
    quint8     cmd;
    QByteArray data;
};

// ─────────────────────────────────────────────────
// 帧构建工具函数
// ─────────────────────────────────────────────────
QByteArray buildFrame(quint8 cmd, const QByteArray &data);

// ─────────────────────────────────────────────────
// 状态机解析器
// 用法：不断调用 feed()，收到完整帧时发出 frameReady 信号
// ─────────────────────────────────────────────────
class FrameParser : public QObject
{
    Q_OBJECT
public:
    explicit FrameParser(QObject *parent = nullptr);
    void feed(const QByteArray &bytes);  // 喂入原始字节

signals:
    void frameReady(SensorFrame frame);  // 完整帧到达

private:
    enum State {
        HEADER1, HEADER2, CMD, LEN, DATA, CHECKSUM, TAIL
    };

    State   m_state    = HEADER1;
    quint8  m_cmd      = 0;
    quint8  m_dataLen  = 0;
    quint8  m_checksum = 0;
    QByteArray m_buf;
};
