#include "scrollablechartview.h"

ScrollableChartView::ScrollableChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
{
    setRenderHint(QPainter::Antialiasing);
    setCursor(Qt::OpenHandCursor);
}

void ScrollableChartView::setFollowing(bool f)
{
    if (m_following == f) return;
    m_following = f;
    emit followingChanged(f);
}

// 被对方图表的 xRangeChanged 调用：只改轴，不再发信号，避免循环
void ScrollableChartView::setXRange(double xMin, double xMax)
{
    if (m_xAxis) m_xAxis->setRange(xMin, xMax);
}

void ScrollableChartView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPos  = e->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    // 不调用基类，禁止橡皮筋选择框
}

void ScrollableChartView::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_dragging || !m_xAxis) return;

    QPoint delta      = e->pos() - m_lastPos;
    m_lastPos         = e->pos();
    double visRange   = m_xAxis->max() - m_xAxis->min();
    if (width() < 1 || visRange < 0.1) return;

    double shift  = -static_cast<double>(delta.x()) * (visRange / width());
    double newMin = m_xAxis->min() + shift;
    double newMax = m_xAxis->max() + shift;

    // 边界钳制：不能超出数据范围
    double maxBound = m_dataMaxT + 5.0;
    if (newMax > maxBound) { newMax = maxBound; newMin = newMax - visRange; }
    if (newMin < 0.0)      { newMin = 0.0;      newMax = newMin + visRange; }

    m_xAxis->setRange(newMin, newMax);

    if (m_following) {
        m_following = false;
        emit followingChanged(false);
    }
    emit xRangeChanged(newMin, newMax);
}

void ScrollableChartView::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
    }
}

void ScrollableChartView::wheelEvent(QWheelEvent *e)
{
    if (!m_xAxis) { e->ignore(); return; }

    double factor  = e->angleDelta().y() > 0 ? 0.75 : 1.33;
    double center  = (m_xAxis->min() + m_xAxis->max()) / 2.0;
    double range   = qBound(10.0, (m_xAxis->max() - m_xAxis->min()) * factor, 3600.0);
    double newMin  = center - range / 2.0;
    double newMax  = center + range / 2.0;

    if (newMin < 0.0) { newMin = 0.0; newMax = range; }
    m_xAxis->setRange(newMin, newMax);

    if (m_following) {
        m_following = false;
        emit followingChanged(false);
    }
    emit xRangeChanged(newMin, newMax);
    e->accept();
}
