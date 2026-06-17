#pragma once
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QMouseEvent>
#include <QWheelEvent>

class ScrollableChartView : public QChartView
{
    Q_OBJECT
public:
    explicit ScrollableChartView(QChart *chart, QWidget *parent = nullptr);

    void setXAxis(QValueAxis *axis)   { m_xAxis = axis; }
    void setDataMaxTime(double t)     { m_dataMaxT = t; }
    bool isFollowing() const          { return m_following; }
    void setFollowing(bool f);

signals:
    void xRangeChanged(double xMin, double xMax); // 用户拖动/缩放时发出
    void followingChanged(bool following);

public slots:
    void setXRange(double xMin, double xMax);     // 由对方图表调用，不重新发信号

protected:
    void mousePressEvent  (QMouseEvent *e) override;
    void mouseMoveEvent   (QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent       (QWheelEvent *e) override;

private:
    QValueAxis *m_xAxis    = nullptr;
    bool        m_following = true;
    bool        m_dragging  = false;
    QPoint      m_lastPos;
    double      m_dataMaxT  = 600.0;
};
