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

#include "weekheadview.h"
#include "calendardbus.h"
#include "customframe.h"
#include "scheduledatamanage.h"

#include <DPalette>
#include <DHiDPIHelper>

#include <QHBoxLayout>
#include <QPainter>
#include <QEvent>
#include <QDebug>
#include <QMessageBox>
#include <QTime>
#include <QQueue>
#include <QMenu>
#include <QMouseEvent>
#include <QPainterPath>

DGUI_USE_NAMESPACE
CWeekHeadView::CWeekHeadView(QWidget *parent)
    : DFrame(parent)
{
    setContentsMargins(0, 0, 0, 0);
    m_DBusInter = new CalendarDBus("com.deepin.api.LunarCalendar",
                                   "/com/deepin/api/LunarCalendar",
                                   QDBusConnection::sessionBus(), this);
    if (!queue)
        queue = new QQueue<int>;

    if (!lunarCache)
        lunarCache = new QMap<QDate, CaLunarDayInfo>;

    if (!emptyCaLunarDayInfo)
        emptyCaLunarDayInfo = new CaLunarDayInfo;

    m_dayNumFont.setWeight(QFont::Medium);
    m_dayNumFont.setPixelSize(DDECalendar::FontSizeSixteen);
    m_monthFont.setWeight(QFont::Medium);
    m_monthFont.setPixelSize(DDECalendar::FontSizeTwenty);

    // cells grid
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->setMargin(0);
    hboxLayout->setSpacing(0);

    m_monthLabel = new CustomFrame(this);
    m_monthLabel->setFixedWidth(DDEWeekCalendar::WMCellHeadrWidth - 5);
    m_monthLabel->setRoundState(true, false, false, false);
    QFont mlabelF;
    mlabelF.setWeight(QFont::Medium);
    mlabelF.setPixelSize(DDECalendar::FontSizeTwenty);
    m_monthLabel->setTextFont(mlabelF);
    hboxLayout->addWidget(m_monthLabel);
    hboxLayout->setStretch(0, 0);
    hboxLayout->setSpacing(0);

    for (int c = 0; c != DDEWeekCalendar::AFewDaysofWeek; ++c) {
        QWidget *cell = new QWidget(this);
        cell->installEventFilter(this);
        cell->setFocusPolicy(Qt::ClickFocus);
        hboxLayout->addWidget(cell);
        m_cellList.append(cell);
    }

    setLayout(hboxLayout);

    connect(this, &CWeekHeadView::dateSelected, this, &CWeekHeadView::handleCurrentDateChanged);
    setFrameRounded(false);

    m_monthList.append( "一月" );
    m_monthList.append( "二月");
    m_monthList.append( "三月" );
    m_monthList.append( "四月" );
    m_monthList.append( "五月" );
    m_monthList.append( "六月" );
    m_monthList.append( "七月" );
    m_monthList.append( "八月");
    m_monthList.append( "九月" );
    m_monthList.append( "十月" );
    m_monthList.append( "十一月");
    m_monthList.append( "十二月");
}

CWeekHeadView::~CWeekHeadView()
{

    if (lunarCache !=nullptr) {
        delete lunarCache;
        lunarCache = nullptr;
    }
    if (emptyCaLunarDayInfo != nullptr) {
        delete  emptyCaLunarDayInfo;
        emptyCaLunarDayInfo = nullptr;
    }
    if (queue != nullptr) {
        delete queue;
        queue = nullptr;
    }
}

void CWeekHeadView::handleCurrentDateChanged(const QDate date, const CaLunarDayInfo &detail)
{
    Q_UNUSED(detail);

    if (date != m_currentDate) {
        setCurrentDate(date);
    }
}

void CWeekHeadView::setFirstWeekday(int weekday)
{
    m_firstWeekDay = weekday;
    updateDate();
}

int CWeekHeadView::getDateType(const QDate &date)
{
    const int currentIndex = getDateIndex(date);
    const CaLunarDayInfo info = getCaLunarDayInfo(currentIndex);
    const int dayOfWeek = date.dayOfWeek();
    bool weekends = dayOfWeek == DDEWeekCalendar::FirstDayofWeekend || dayOfWeek == DDEWeekCalendar::AFewDaysofWeek;
    bool isCurrentMonth = m_currentDate.month() == date.month();
    bool isFestival = !info.mSolarFestival.isEmpty() || !info.mLunarFestival.isEmpty();
    int resultFlag = 0;

    if (!isCurrentMonth)
        resultFlag |= SO_MNotCurrentMonth;
    if (isFestival)
        resultFlag |= SO_MFestival;
    if (weekends)
        resultFlag |= SO_MWeekends;

    return resultFlag;
}

void CWeekHeadView::setTheMe(int type)
{
    m_themetype = type;

    if (type == 0 || type == 1) {
        QColor textC = "#000000";
        QColor textbC(230, 238, 242);

        m_monthLabel->setBColor(textbC);
        m_monthLabel->setTextColor(textC);

        m_backgroundCircleColor = "#0081FF";
        m_backgroundShowColor = "#2CA7F8";
        m_backgroundShowColor.setAlphaF(0.4);
        m_Background_Weekend_Color = "#DAE4ED";

        m_defaultTextColor = "#6F6F6F";
        m_currentDayTextColor = "#FFFFFF";
        m_defaultLunarColor = "#898989";
        m_weekendsTextColor = CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor();
        m_currentMonthColor = "#000000";
        m_backgroudColor = "#E6EEF2";
        m_solofestivalLunarColor = "#4DFF7272";
    } else if (type == 2) {
        QColor textbC = "#82AEC1";
        textbC.setAlphaF(0.1);
        m_monthLabel->setBColor(textbC);
        m_monthLabel->setTextColor("#BF1D63");
        m_backgroundCircleColor = "#0059D2";
        m_backgroundShowColor = "#002AAF";
        m_backgroundShowColor.setAlphaF(0.4);
        m_Background_Weekend_Color = "#333D4A";

        m_defaultTextColor = "#C0C6D4";
        m_currentDayTextColor = "#C0C6D4";
        m_defaultLunarColor = "#6886BA";
        m_weekendsTextColor = CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor();
        m_currentMonthColor = "#000000";
        m_backgroudColor = "#82AEC1";
        m_backgroudColor.setAlphaF(0.1);
        m_solofestivalLunarColor = "#4DFF7272";
    }

    for (int i = 0; i != DDEWeekCalendar::AFewDaysofWeek; ++i)
        m_cellList.at(i)->update();
}

void CWeekHeadView::setWeekDay(QVector<QDate> vDays)
{
    if (vDays.count() != DDEWeekCalendar::AFewDaysofWeek)
        return;

    for (int i = 0; i != DDEWeekCalendar::AFewDaysofWeek; ++i)
        m_days[i] = vDays[i];
}

void CWeekHeadView::setMounthLabelWidth(int w, int rw)
{
    m_monthW = w + 1;
    m_fixwidth = rw;
}

void CWeekHeadView::setCurrentDate(const QDate date)
{
    qDebug() << "set current date " << date;

    if (date == m_currentDate) {
        return;
    }
    m_currentDate = date;

    emit signalcurrentDateChanged(m_currentDate);
    // to refresh lunar calendar
    updateCurrentLunar(getCaLunarDayInfo(getDateIndex(m_currentDate)));
    QLocale locale;
    m_monthLabel->setTextStr(locale.monthName(date.month(), QLocale::ShortFormat));
}

void CWeekHeadView::setLunarVisible(bool visible)
{
    int state = int(m_showState);

    if (visible)
        state |= ShowLunar;
    else
        state &= ~ShowLunar;

    m_showState = ShowState(state);
    update();
}

void CWeekHeadView::setCellSelectable(bool selectable)
{
    if (selectable == m_cellSelectable)
        return;

    m_cellSelectable = selectable;

    emit cellSelectableChanged(m_cellSelectable);
}

int CWeekHeadView::getDateIndex(const QDate &date) const
{
    for (int i = 0; i != DDEWeekCalendar::AFewDaysofWeek; ++i)
        if (m_days[i] == date)
            return i;

    return 0;
}

bool CWeekHeadView::eventFilter(QObject *o, QEvent *e)
{
    QWidget *cell = qobject_cast<QWidget *>(o);

    if (cell && m_cellList.contains(cell)) {
        if (e->type() == QEvent::Paint) {
            paintCell(cell);
        } else if (e->type() == QEvent::MouseButtonPress) {
        } else if (e->type() == QEvent::MouseButtonDblClick) {
            const int pos = m_cellList.indexOf(cell);
            emit signalsViewSelectDate(m_days[pos]);
        }
    }

    return false;
}

void CWeekHeadView::updateDate()
{
    const int currentIndex = getDateIndex(m_currentDate);
    setSelectedCell(currentIndex);
    update();
}

void CWeekHeadView::updateCurrentLunar(const CaLunarDayInfo &info)
{
    if (!info.mLunarFestival.isEmpty()) {
        emit currentFestivalChanged(info.mLunarFestival);
    } else if (!info.mTerm.isEmpty()) {
        emit currentFestivalChanged(info.mTerm);
    } else if (!info.mSolarFestival.isEmpty()) {
        QStringList tmpFestival = info.mSolarFestival.split(" ");

        if (tmpFestival.length() >= 3) {
            emit currentFestivalChanged(QString("%1 %2").arg(tmpFestival[0]).arg(tmpFestival[1]));
        } else {
            emit currentFestivalChanged(info.mSolarFestival);
        }
    } else {
        emit currentFestivalChanged("");
    }

    QVector<QDate> vdate;
    QVector<CaLunarDayInfo> vdetail;

    for (int i = 0; i < DDEWeekCalendar::AFewDaysofWeek; i++) {
        vdate.append(m_days[i]);
        vdetail.append(getCaLunarDayInfo(getDateIndex(m_days[i])));
    }

    emit signalcurrentLunarDateChanged(vdate, vdetail, 1);
    updateDate();
}

const QString CWeekHeadView::getCellDayNum(int pos)
{
    return QString::number(m_days[pos].day());
}

const QDate CWeekHeadView::getCellDate(int pos)
{
    return m_days[pos];
}

const QString CWeekHeadView::getLunar(int pos)
{
    CaLunarDayInfo info = getCaLunarDayInfo(pos);

    if (info.mLunarDayName == "初一") {
        info.mLunarDayName =  info.mLunarMonthName;
    }

    if (info.mTerm.isEmpty())
        return info.mLunarDayName;

    return info.mTerm;
}

const CaLunarDayInfo CWeekHeadView::getCaLunarDayInfo(int pos)
{
    const QDate date = m_days[pos];

    if (lunarCache->contains(date)) {
        return lunarCache->value(date);
    }

    if (lunarCache->size() > 300)
        lunarCache->clear();

    queue->push_back(pos);

    QTimer::singleShot(300, this, SLOT(getDbusData()));

    return *emptyCaLunarDayInfo;
}

void CWeekHeadView::getDbusData()
{
    if (queue->isEmpty())
        return;

    const int pos = queue->head();
    queue->pop_front();
    const QDate date = m_days[pos];

    if (!date.isValid()) {
        return;
    }

    CaLunarDayInfo currentDayInfo;

    if (!lunarCache->contains(date)) {
        bool o1 = true;
        QDBusReply<CaLunarMonthInfo> reply = m_DBusInter->GetLunarMonthCalendar(date.year(), date.month(), false, o1);

        QDate cacheDate;
        cacheDate.setDate(date.year(), date.month(), 1);
        foreach (const CaLunarDayInfo &dayInfo, reply.value().mCaLunarDayInfo) {
            lunarCache->insert(cacheDate, dayInfo);

            if (date == m_currentDate) {
                currentDayInfo = dayInfo;
            }
            cacheDate = cacheDate.addDays(1);
        }
    } else {
        currentDayInfo = lunarCache->value(date);
    }

    m_cellList.at(pos)->update();

    // refresh   lunar info
    if (date == m_currentDate) {
        updateCurrentLunar(currentDayInfo);
    }
}

void CWeekHeadView::paintCell(QWidget *cell)
{
    m_weekendsTextColor = CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor();
    const QRect rect(0, 0, cell->width(), cell->height());
    const int pos = m_cellList.indexOf(cell);
    const bool isCurrentDay = getCellDate(pos) == QDate::currentDate();
    const bool isSelectedCell = isCurrentDay;
    int d = m_days[pos].dayOfWeek();

    QPainter painter(cell);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    //根据周几设置不一样的背景色
    if (d == DDEWeekCalendar::FirstDayofWeekend || d == DDEWeekCalendar::AFewDaysofWeek) {
        painter.setBrush(QBrush(m_Background_Weekend_Color));
    } else {
        painter.setBrush(QBrush(m_backgroudColor));
    }
    if (d != 6) {
        painter.drawRect(rect);//画矩形
    } else {
        int labelwidth = cell->width();
        int labelheight = cell->height();
        QPainterPath painterPath;
        painterPath.moveTo(m_radius, 0);

        painterPath.lineTo(0, 0);
        painterPath.lineTo(0, m_radius);
        painterPath.lineTo(0, labelheight - m_radius);
        painterPath.lineTo(0, labelheight);
        painterPath.lineTo(m_radius, labelheight);
        painterPath.lineTo(labelwidth - m_radius, labelheight);
        painterPath.lineTo(labelwidth, labelheight);
        painterPath.lineTo(labelwidth, labelheight - m_radius);
        painterPath.lineTo(labelwidth, m_radius);
        painterPath.arcTo(QRect(labelwidth - m_radius * 2, 0, m_radius * 2, m_radius * 2), 0, 90);
        painterPath.lineTo(m_radius, 0);
        painterPath.closeSubpath();
        painter.drawPath(painterPath);
    }
    //绘制分割线
    if (d != DDEWeekCalendar::FirstDayofWeekend && d != DDEWeekCalendar::AFewDaysofWeek) {
        QPoint point_begin(cell->width(), 0);
        QPoint point_end(cell->width(), cell->height());
        painter.save();
        QColor m_linecolor = "#000000";
        m_linecolor.setAlphaF(0.1);
        painter.setPen(m_linecolor);
        painter.drawLine(point_begin, point_end);
        painter.restore();
    }

    int bw = (cell->width() - 104) / 2;
    int bh = (cell->height() - 26) / 2;

    if (bw < 0) {
        bw = 2;
    }
    if (bh < 0) {
        bh = 2;
    }
    if (isSelectedCell) {
        if (m_showState & ShowLunar) {
            QRect fillRect(bw - 2, bh, 26, 26);
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(QBrush(CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor()));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(fillRect);
            painter.restore();
        } else {
            QRect fillRect(cell->width() - (cell->width()/2) + 1, bh - 1, 26, 26);
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(QBrush(CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor()));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(fillRect);
            painter.restore();
        }

    }
    QLocale locale;
    const QString dayNum = getCellDayNum(pos);
    const QString dayLunar = getLunar(pos);
    QString dayWeek = locale.dayName(d ? d : DDEWeekCalendar::AFewDaysofWeek, QLocale::ShortFormat);

    painter.save();
    painter.setPen(Qt::SolidLine);
    // draw text of day
    if (isSelectedCell) {
        painter.setPen(m_currentDayTextColor);
    } else if (isCurrentDay) {
        painter.setPen(m_weekendsTextColor);
    } else {
        if (d == DDEWeekCalendar::FirstDayofWeekend || d == DDEWeekCalendar::AFewDaysofWeek)
            painter.setPen(m_weekendsTextColor);
        else
            painter.setPen(m_defaultTextColor);
    }

    painter.setFont(m_dayNumFont);

    if (m_showState & ShowLunar) {
        painter.drawText(QRect(bw - 1, bh, 24, 24), Qt::AlignCenter, dayNum);
        if (d == DDEWeekCalendar::FirstDayofWeekend || d == DDEWeekCalendar::AFewDaysofWeek)
            painter.setPen(m_weekendsTextColor);
        else
            painter.setPen(m_defaultTextColor);
        painter.drawText(QRect(bw + 24, bh, 30, 25), Qt::AlignCenter, dayWeek);
    } else {
        QFontMetrics fm1 = painter.fontMetrics();
        painter.drawText(QRect(cell->width() - (cell->width()/2) - 4, bh - 1, 36, 26), Qt::AlignCenter, dayNum);
        if (d == DDEWeekCalendar::FirstDayofWeekend || d == DDEWeekCalendar::AFewDaysofWeek)
            painter.setPen(m_weekendsTextColor);
        else
            painter.setPen(m_defaultTextColor);

        QFontMetrics fm = painter.fontMetrics();

        while (fm.width(dayWeek) > cell->width()/2)
            dayWeek.chop(1);

        painter.drawText(QRect(0, bh, (cell->width()/2), 26), Qt::AlignRight, dayWeek);
    }
    // draw text of day type
    if (m_showState & ShowLunar) {
        if (cell->width() > 100) {
            if (d == DDEWeekCalendar::FirstDayofWeekend || d == DDEWeekCalendar::AFewDaysofWeek)
                painter.setPen(m_weekendsTextColor);
            else
                painter.setPen(m_defaultLunarColor);

            if (cell->width() < 132) {
                QString str_dayLunar = nullptr;
                if (dayLunar.count() > 2) {
                    for (int i = 0; i < 2; i++) {
                        str_dayLunar.append(dayLunar.at(i));
                    }
                    str_dayLunar.append("...");
                } else {
                    str_dayLunar = dayLunar;
                }
                painter.drawText(QRect(bw + 52 + 10, bh, 50, 25), Qt::AlignLeft, str_dayLunar);
            } else {
                painter.drawText(QRect(bw + 52 + 10, bh, 50, 25), Qt::AlignLeft, dayLunar);
            }
            CaLunarDayInfo dayInfo = getCaLunarDayInfo(pos);
        }
    }
    painter.restore();
    painter.end();
}

void CWeekHeadView::cellClicked(QWidget *cell)
{
    if (!m_cellSelectable)
        return;

    const int pos = m_cellList.indexOf(cell);
    if (pos == -1)
        return;

    setSelectedCell(pos);

    // my gift eggs
    static int gift = 0;

    if (m_days[pos] == QDate(1993, 7, 28))
        if (++gift == 10)
            QMessageBox::about(this, "LinuxDeepin", "by shibowen <sbw@sbw.so> :P");
}

void CWeekHeadView::setSelectedCell(int index)
{
    if (m_selectedCell == index)
        return;

    const int prevPos = m_selectedCell;
    m_selectedCell = index;

    m_cellList.at(prevPos)->update();
    m_cellList.at(index)->update();
    emit dateSelected(m_days[index], getCaLunarDayInfo(index));
}

int CWeekHeadView::checkDay(int weekday)
{
    // check the week, calculate the correct order in the custom.
    if (weekday <= 0)
        return weekday += DDEWeekCalendar::AFewDaysofWeek;

    if (weekday > DDEWeekCalendar::AFewDaysofWeek)
        return weekday -= DDEWeekCalendar::AFewDaysofWeek;

    return weekday;
}

void CWeekHeadView::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit signaleSchedulHide();
}
