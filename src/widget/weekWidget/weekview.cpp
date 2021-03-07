/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     kirigaya <kirigaya@mkacg.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "weekview.h"
#include "scheduledatamanage.h"
#include "constants.h"

#include <DWidget>
#include <DHiDPIHelper>

#include <QHBoxLayout>
#include <QPainter>
#include <QBrush>
#include <QEvent>
#include <QMessageBox>
#include <QWheelEvent>
#include <QtGlobal>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE

CWeekView::CWeekView(QWidget *parent)
    : QWidget(parent)
{
    m_dayNumFont.setPixelSize(DDECalendar::FontSizeSixteen);
    m_dayNumFont.setWeight(QFont::Light);
    // cells grid
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->setMargin(0);
    hboxLayout->setSpacing(0);

    for (int c = 0; c != DDEWeekCalendar::NumWeeksDisplayed; ++c) {
        QWidget *cell = new QWidget;
        cell->setFixedSize(DDEWeekCalendar::WWeekCellWidth, DDEWeekCalendar::WWeekCellHeight);
        cell->installEventFilter(this);
        cell->setFocusPolicy(Qt::ClickFocus);
        hboxLayout->addWidget(cell);
        m_cellList.append(cell);
    }
    setLayout(hboxLayout);
    setMinimumWidth(150);
}

CWeekView::~CWeekView()
{
    for (int i = 0; i < DDEWeekCalendar::NumWeeksDisplayed; i++) {
        m_cellList.at(i)->removeEventFilter(this);
        delete m_cellList.at(i);
    }
    m_cellList.clear();
}

void CWeekView::setFirstWeekDay(int weekday)
{
    m_firstWeekDay = weekday;
    updateDate();
}

void CWeekView::setCurrentDate(const QDate date)
{
    m_selectDate = date;
    m_selectedCell = 0;
    updateDate();
    update();
}

void CWeekView::setTheMe(int type)
{
    m_themetype = type;

    if (type == 0 || type == 1) {
        m_defaultTextColor = Qt::black;
        m_backgrounddefaultColor = Qt::white;
        m_currentDayTextColor = Qt::white;
        m_backgroundcurrentDayColor = CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor();
        m_fillColor = "#FFFFFF";
    } else if (type == 2) {
        m_defaultTextColor = "#C0C6D4";
        m_backgrounddefaultColor = "#FFFFFF";
        m_backgrounddefaultColor.setAlphaF(0.05);
        m_currentDayTextColor = "#B8D3FF";
        m_backgroundcurrentDayColor = CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor();
        m_fillColor = "#000000";
        m_fillColor.setAlphaF(0.05);
    }
}

void CWeekView::setsearchfalg(bool flag)
{
    m_searchfalg = flag;
}
void CWeekView::slotprev()
{
    if (m_selectDate.year() >= DDECalendar::QueryEarliestYear) {
        QDate date = m_selectDate.addDays(-6);
        setCurrentDate(date);
    } else {
    }
}

void CWeekView::slotnext()
{
    QDate date  = m_selectDate.addDays(8);;
    setCurrentDate(date);
}
void CWeekView::paintCell(QWidget *cell)
{
    const QRect rect(0, 0, cell->width(), cell->height());
    const int pos = m_cellList.indexOf(cell);
    //计算当前日期周数
    int weekNumber = QDate::currentDate().weekNumber();
    //In accordance with ISO 8601, weeks start on Monday and the first
    if (QDate::currentDate().dayOfWeek() == DDEWeekCalendar::AFewDaysofWeek) {
        ++weekNumber;
    }

    const bool isCurrentDay = m_days[pos].addDays(3).weekNumber() == weekNumber &&
                              m_days[pos].addDays(3).year() == QDate::currentDate().year();
    const bool isSelectDay = m_days[pos].addDays(m_weekAddDay).weekNumber() == m_selectDate.addDays(m_weekAddDay).weekNumber();

    QPainter painter(cell);

    painter.save();
    painter.setRenderHints(QPainter::HighQualityAntialiasing);
    painter.setBrush(QBrush(m_fillColor));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect);//画矩形
    painter.restore();
    painter.setPen(Qt::SolidLine);

    const QString dayNum = QString::number(m_days[pos].addDays(3).weekNumber());

    if (m_days[pos].year() < DDECalendar::QueryEarliestYear && dayNum != "1")
        return;

    if (isSelectDay) {
        QRect fillRect((cell->width() - 24) / 2, (cell->height() - 32) / 2 + 4, 24, 24);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QBrush(CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor()));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(fillRect);
        painter.restore();
        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        painter.setPen(m_currentDayTextColor);
        painter.setFont(m_dayNumFont);
        painter.drawText(QRect(0, 0, cell->width(), cell->height()), Qt::AlignCenter, dayNum);
    } else {
        if (isCurrentDay) {
            painter.setPen(m_backgroundcurrentDayColor);
        } else {
            painter.setPen(m_defaultTextColor);
        }
        painter.setFont(m_dayNumFont);
        painter.drawText(QRect(0, 0, cell->width(), cell->height()), Qt::AlignCenter, dayNum);
    }
    painter.end();
}

bool CWeekView::eventFilter(QObject *o, QEvent *e)
{
    QWidget *cell = qobject_cast<QWidget *>(o);

    if (cell && m_cellList.contains(cell)) {
        if (e->type() == QEvent::Paint) {
            paintCell(cell);
        } else if (e->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(e);
            if (mouseEvent->button() ==Qt::LeftButton) {
                cellClicked(cell);
            }
        }
    }

    return false;
}

void CWeekView::cellClicked(QWidget *cell)
{
    if (!m_cellSelectable)
        return;

    const int pos = m_cellList.indexOf(cell);

    if (pos == -1)
        return;

    setSelectedCell(pos);
    update();
}

void CWeekView::setSelectedCell(int index)
{
    if (m_selectedCell == index)
        return;

    const int prevPos = m_selectedCell;
    m_selectedCell = index;

    m_cellList.at(prevPos)->update();
    m_cellList.at(index)->update();
    m_selectDate = m_days[index];
    const QString dayNum = QString::number(m_days[index].addDays(3).weekNumber());

    if (m_days[index].year() < DDECalendar::QueryEarliestYear && dayNum != "1")
        return;

    emit signalsSelectDate(m_days[index], m_days[index].addDays(m_weekAddDay));
}

void CWeekView::updateDate()
{
    m_weekAddDay = (m_selectDate.dayOfWeek() + m_firstWeekDay) % DDEWeekCalendar::AFewDaysofWeek;
    QDate weekfirstDay = m_selectDate.addDays(-m_weekAddDay);
    m_days[4] = weekfirstDay;

    for (int i(0); i < 4; ++i) {
        m_days[i] = weekfirstDay.addDays(-(4 - i) * DDEWeekCalendar::AFewDaysofWeek);
    }
    for (int i(5); i != DDEWeekCalendar::NumWeeksDisplayed; ++i) {
        m_days[i] = weekfirstDay.addDays((i - 4) * DDEWeekCalendar::AFewDaysofWeek);
    }

    setSelectedCell(4);
    update();
}
void CWeekView::resizeEvent(QResizeEvent *event)
{
    int w = width() / DDEWeekCalendar::NumWeeksDisplayed;
    int h = height();
    int ww = 36;

    if (w >= ww) {
        for (int c = 0; c != DDEWeekCalendar::NumWeeksDisplayed; ++c) {
            m_cellList[c]->setFixedSize(w, h);
            m_cellList[c]->update();
        }
        for (int i = 0; i < DDEWeekCalendar::NumWeeksDisplayed; i++) {
            m_cellList[i]->setVisible(true);
            m_cellList[i]->update();
        }
    } else {
        for (int c = 0; c != DDEWeekCalendar::NumWeeksDisplayed; ++c) {
            m_cellList[c]->setFixedSize(ww, h);
            m_cellList[c]->update();
        }
        int t_num = qRound((ww * DDEWeekCalendar::NumWeeksDisplayed - width()) / ww / 2.0);
        QVector<bool> vindex;
        vindex.resize(10);
        vindex.fill(true);

        for (int i = 0; i < t_num; i++) {
            vindex[i] = false;
            vindex[9 - i] = false;
        }
        for (int i = 0; i < DDEWeekCalendar::NumWeeksDisplayed; i++) {
            m_cellList[i]->setVisible(vindex[i]);
            m_cellList[i]->update();
        }
    }
    QWidget::resizeEvent(event);
}
void CWeekView::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event);
    bool isDragging = false;
    //
    emit signalIsDragging(isDragging);
    //判断是否是拖拽状态
    if (!isDragging) {
        if (event->delta() < 0) {
            slotnext();
        } else {
            slotprev();
        }
    }

}
