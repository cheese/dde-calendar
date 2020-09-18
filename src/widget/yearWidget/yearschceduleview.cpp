/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
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
#include "yearschceduleview.h"
#include "schcedulectrldlg.h"
#include "myschceduleview.h"
#include "schceduledlg.h"
#include "scheduledatamanage.h"
#include "constants.h"

#include <DMessageBox>
#include <DPushButton>
#include <DHiDPIHelper>
#include <DPalette>

#include <QAction>
#include <QMenu>
#include <QListWidget>
#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>
#include <QStylePainter>
#include <QRect>
#include <QMouseEvent>

DGUI_USE_NAMESPACE

const QString fontfamily = QStringLiteral("SourceHanSansSC-Medium");

CYearSchceduleItem::CYearSchceduleItem(QWidget *parent)
    : DLabel(parent)
{

}

void CYearSchceduleItem::setBackgroundColor(QColor color1)
{
    m_Backgroundcolor = color1;
}

void CYearSchceduleItem::setStateColor(QColor color1)
{
    m_Statecolor = color1;
}

void CYearSchceduleItem::setText( QColor tcolor, QFont font)
{
    m_ttextcolor = tcolor;
    m_tfont = font;
}

void CYearSchceduleItem::setTimeC(QColor tcolor, QFont font)
{
    m_timecolor = tcolor;
    m_timefont = font;
}

void CYearSchceduleItem::setData( ScheduleDtailInfo vScheduleInfo)
{
    m_ScheduleInfo = vScheduleInfo;
    update();
}

void CYearSchceduleItem::paintEvent( QPaintEvent *e )
{
    Q_UNUSED(e);
    int labelwidth = width();
    int labelheight = height();

    QPainter painter(this);
    QRect fillRect = QRect(0, 0, labelwidth, labelheight);
    painter.setRenderHints(QPainter::HighQualityAntialiasing);
    QColor bcolor = m_Backgroundcolor;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setBrush(QBrush(bcolor));
    painter.setPen(Qt::NoPen);
    painter.drawRect(fillRect);
    painter.restore();
    //圆点
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setBrush(QBrush(m_Statecolor));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QRect(10, (labelheight - 8) / 2, 8, 8));
    painter.restore();

    QString str;
    //左边文字
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setPen(m_ttextcolor);
    painter.setFont(m_tfont);
    QFontMetrics fm = painter.fontMetrics();
    QString tStitlename = m_ScheduleInfo.titleName;
    tStitlename.replace("\n", "");
    str = tStitlename;
    int tilenameW = labelwidth - 90;
    QString tstr;

    for (int i = 0; i < str.count(); i++) {
        tstr.append(str.at(i));
        int widthT = fm.width(tstr) + 5;
        if (widthT >= tilenameW) {
            tstr.chop(1);
            break;
        }
    }
    if (tstr != str) {
        tstr = tstr + "...";
    }

    painter.drawText(QRect(28, 0, tilenameW, labelheight - 2), Qt::AlignLeft | Qt::AlignVCenter, tstr);
    painter.restore();
    //右边时间
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setPen(m_timecolor);
    painter.setFont(m_timefont);
    QLocale locale;

    if (locale.language() == QLocale::Chinese) {
        if (m_ScheduleInfo.allday) {
            str = tr("All Day");
        } else {
            str = m_ScheduleInfo.beginDateTime.time().toString("ap h") + ("时");
        }
    } else {
        if (m_ScheduleInfo.allday) {
            str = tr("All Day");
        } else {
            str = m_ScheduleInfo.beginDateTime.time().toString("ap h:mm");
        }
    }
    QFontMetrics fm2 = painter.fontMetrics();
    painter.drawText(QRect(labelwidth - 60, 0, 50, labelheight - 2), Qt::AlignRight | Qt::AlignVCenter, str);
    painter.restore();
}

CYearSchceduleView::CYearSchceduleView(QWidget *parent)
    : DWidget(parent)
{
    setContentsMargins(10, 10, 10, 10);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
}

CYearSchceduleView::~CYearSchceduleView()
{

}

void CYearSchceduleView::setSoloDay(QString soloday)
{
    m_soloDay = soloday;
}

bool YScheduleDateThan(const ScheduleDtailInfo &s1, const ScheduleDtailInfo &s2)
{
    QDate bdate1 = s1.beginDateTime.date();
    QDate edate1 = s1.endDateTime.date();
    QDate bdate2 = s2.beginDateTime.date();
    QDate edate2 = s2.endDateTime.date();

    if (bdate1 != edate1 && bdate2 == edate2) {
        return true;
    } else if (bdate1 == edate1 && bdate2 != edate2) {
        return false;
    } else if (bdate1 != edate1 && bdate2 != edate2) {
        return bdate1 < bdate2;
    } else {
        if (s1.beginDateTime == s2.beginDateTime) {
            return s1.titleName < s2.titleName;
        } else {
            return s1.beginDateTime < s2.beginDateTime;
        }
    }
}
bool YScheduleDaysThan(const ScheduleDtailInfo &s1, const ScheduleDtailInfo &s2)
{
    return s1.beginDateTime.date().daysTo(s1.endDateTime.date()) > s2.beginDateTime.date().daysTo(s2.endDateTime.date());
}

void CYearSchceduleView::setData(QVector<ScheduleDtailInfo> &vListData)
{
    QVector<ScheduleDtailInfo> valldayListData, vDaylistdata;

    for (int i = 0; i < vListData.count(); i++) {
        if (vListData.at(i).allday) {
            valldayListData.append(vListData.at(i));
        } else {
            vDaylistdata.append(vListData.at(i));
        }
    }

    std::sort(valldayListData.begin(), valldayListData.end(), YScheduleDaysThan);
    std::sort(valldayListData.begin(), valldayListData.end(), YScheduleDateThan);
    std::sort(vDaylistdata.begin(), vDaylistdata.end(), YScheduleDaysThan);
    std::sort(vDaylistdata.begin(), vDaylistdata.end(), YScheduleDateThan);

    for (int i = 0; i < valldayListData.count(); i++) {
        QVector<ScheduleDtailInfo>::iterator iter = valldayListData.begin();
        if (valldayListData.at(i).type.ID == DDECalendar::FestivalTypeID) {
            ScheduleDtailInfo moveDate;
            moveDate = valldayListData.at(i);
            valldayListData.removeAt(i);
            valldayListData.insert(iter, moveDate);
        }
    }

    m_vlistData.clear();
    m_vlistData.append(valldayListData);
    m_vlistData.append(vDaylistdata);

    if (m_vlistData.size() > DDEYearCalendar::YearScheduleListMaxcount) {
        QVector<ScheduleDtailInfo> vTlistData;
        for (int i = 0; i < 4; i++) {
            if (m_vlistData.at(i).beginDateTime.date() != m_vlistData.at(i).endDateTime.date() && !m_vlistData.at(i).allday) {
                if (m_vlistData.at(i).beginDateTime.date() != m_currentDate) {
                    m_vlistData[i].allday = true;
                }
            }
            vTlistData.append(m_vlistData.at(i));
        }
        ScheduleDtailInfo info;
        info.titleName = "......";
        info.id = -1;
        vTlistData.append(info);
        m_vlistData = vTlistData;
    }
}

void CYearSchceduleView::clearData()
{
    m_vlistData.clear();
    m_soloDay.clear();
    return;
}

int CYearSchceduleView::showWindow()
{
    if (m_soloDay.isEmpty() && m_vlistData.isEmpty()) {
        setFixedSize(130, 45);
    } else {
        setFixedSize(240, 180);
    }
    updateDateShow();

    return this->width();
}

void CYearSchceduleView::setTheMe(int type)
{
    if (type == 0 || type == 1) {
        m_bBackgroundcolor = "#FFFFFF";
        m_bBackgroundcolor.setAlphaF(0.0);
        m_btimecolor = "#414D68";
        m_btimecolor.setAlphaF(0.7);
        m_bttextcolor = "#414D68";
        m_lBackgroundcolor = "#EBEBEB";
        m_lBackgroundcolor.setAlphaF(0.0);
        m_ltextcolor = "#001A2E";
        m_solocolor = "#FF7272";
        m_TBcolor = "#EBEBEB";
        m_borderColor = "#000000";
        m_borderColor.setAlphaF(0.05);
    } else if (type == 2) {
        m_bBackgroundcolor = "#FFFFFF";
        m_bBackgroundcolor.setAlphaF(0.0);
        m_btimecolor = "#C0C6D4";
        m_btimecolor.setAlphaF(0.7);
        m_bttextcolor = "#C0C6D4";
        m_lBackgroundcolor = "#191919";
        m_lBackgroundcolor.setAlphaF(0.00);
        m_ltextcolor = "#C0C6D4";
        m_solocolor = "#FF7272";
        m_solocolor.setAlphaF(0.8);
        m_TBcolor = "#191919";
        m_borderColor = "#FFFFFF";
        m_borderColor.setAlphaF(0.05);
    }
}

void CYearSchceduleView::setDtype(int type, int arrowheight)
{
    m_dtype = type;
    m_arrowheight = arrowheight;
    update();
}

void CYearSchceduleView::setCurrentDate(QDate cdate)
{
    m_currentDate = cdate;
}

QDate CYearSchceduleView::getCurrentDate()
{
    return  m_currentDate;
}

void CYearSchceduleView::adjustPosition(bool ad)
{
    adjustPos = ad;
}

void CYearSchceduleView::updateDateShow()
{
    int sviewNum = 0;

    if (!m_soloDay.isEmpty() || !m_vlistData.isEmpty()) {
        if (!m_soloDay.isEmpty()) {
            if (m_vlistData.size() > 4) {
                sviewNum = DDEYearCalendar::YearScheduleListMaxcount;
            } else {
                sviewNum = m_vlistData.size() + 1;
            }
        } else {
            if (m_vlistData.size() > DDEYearCalendar::YearScheduleListMaxcount) {
                sviewNum = DDEYearCalendar::YearScheduleListMaxcount;
            } else {
                sviewNum = m_vlistData.size();
            }
        }
    }

    if (!m_soloDay.isEmpty() || !m_vlistData.isEmpty())
        setFixedSize(240, 45 + (sviewNum - 1) * 29);
    update();

    return;
}

void CYearSchceduleView::createItemWidget(ScheduleDtailInfo info, int type)
{
    ScheduleDtailInfo &gd = info;
    CSchedulesColor gdcolor = CScheduleDataManage::getScheduleDataManage()->getScheduleColorByType(gd.type.ID);
    CYearSchceduleItem *gwi = new CYearSchceduleItem();
    QFont font;

    font.setWeight(QFont::Medium);
    font.setPixelSize(DDECalendar::FontSizeFourteen);
    gwi->setBackgroundColor(m_bBackgroundcolor);
    QColor scolor = gdcolor.Purecolor;
    scolor.setAlphaF(1.0);

    if (type == 0)
        gwi->setStateColor(gdcolor.splitColor);
    else {
        gwi->setStateColor(m_solocolor);
    }
    gwi->setText(m_bttextcolor, font);
    font.setPixelSize(DDECalendar::FontSizeTwelve);
    gwi->setTimeC(m_btimecolor, font);
    gwi->setFixedSize(m_gradientItemList->width() - 3, 28);
    gwi->setData(gd);

    QListWidgetItem *listItem = new QListWidgetItem;
    listItem->setSizeHint(QSize(m_gradientItemList->width() - 2, 29)); //每次改变Item的高度
    listItem->setFlags(Qt::ItemIsTristate );
    m_gradientItemList->addItem(listItem);
    m_gradientItemList->setItemWidget(listItem, gwi);
    m_labellist.append(gwi);
}

void CYearSchceduleView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    for (int i = 0; i < m_vlistData.size(); ++i) {
        paintItem(m_vlistData.at(i), i, 0);
    }
    if (m_soloDay.isEmpty() && m_vlistData.isEmpty()) {
        paintItem();
    }
    adjustPos = false;
}

void CYearSchceduleView::paintItem(ScheduleDtailInfo info, int index, int type)
{
    int labelwidth = width() - 30;
    int bheight = index * 29 + 10;
    int labelheight = 28;
    ScheduleDtailInfo &gd = info;
    CSchedulesColor gdcolor = CScheduleDataManage::getScheduleDataManage()->getScheduleColorByType(gd.type.ID);
    QFont font;

    font.setWeight(QFont::Medium);
    font.setPixelSize(DDECalendar::FontSizeTwelve);
    QColor scolor = gdcolor.Purecolor;
    scolor.setAlphaF(1.0);

    QPainter painter(this);
    QRect fillRect = QRect(0, 0, width(), height());
    painter.setRenderHints(QPainter::HighQualityAntialiasing);
    QColor bcolor = m_bBackgroundcolor;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setBrush(QBrush(bcolor));
    painter.setPen(Qt::NoPen);
    painter.drawRect(fillRect);
    painter.restore();

    if (gd.id == -1) {
        QString str = "...";

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(m_btimecolor);
        painter.setFont(font);
        if (adjustPos) {
            painter.drawText(QRect(25 - 18, bheight, labelwidth - 80, labelheight - 2), Qt::AlignLeft | Qt::AlignVCenter, str);
        } else {
            painter.drawText(QRect(25, bheight, labelwidth - 80, labelheight - 2), Qt::AlignLeft | Qt::AlignVCenter, str);
        }
        painter.restore();
    } else {
        if (info.id != -1) {
            //圆点
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
            if (type == 0)
                painter.setBrush(QBrush(gdcolor.splitColor));
            else {

                painter.setBrush(QBrush(m_solocolor));
            }
            painter.setPen(Qt::NoPen);

            if (adjustPos) {
                painter.drawEllipse(QRect(25 - 18, bheight + (labelheight - 8) / 2, 8, 8));
            } else {
                painter.drawEllipse(QRect(25, bheight + (labelheight - 8) / 2, 8, 8));
            }
            painter.restore();
        }

        QString str;
        //左边文字
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
        painter.setPen(m_bttextcolor);
        painter.setFont(font);
        QFontMetrics fm = painter.fontMetrics();
        QString tStitlename = gd.titleName;
        tStitlename.replace("\n", "");
        str = tStitlename;
        int tilenameW = labelwidth - 80;
        QString tstr;

        for (int i = 0; i < str.count(); i++) {
            tstr.append(str.at(i));
            int widthT = fm.width(tstr) + 5;
            if (widthT >= tilenameW) {
                tstr.chop(1);
                break;
            }
        }
        if (tstr != str) {
            tstr = tstr + "...";
        }
        if (adjustPos) {
            painter.drawText(QRect(41 - 18, bheight, tilenameW, labelheight - 2), Qt::AlignLeft | Qt::AlignVCenter, tstr);
        } else {
            painter.drawText(QRect(41, bheight, tilenameW, labelheight - 2), Qt::AlignLeft | Qt::AlignVCenter, tstr);
        }
        painter.restore();

        if (info.id != -1) {
            //右边时间
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
            painter.setPen(m_btimecolor);
            painter.setFont(font);
            QLocale locale;

            if (info.allday) {
                str = tr("All Day");
            } else {
                if (m_currentDate > info.beginDateTime.date()) {
                    str = tr("All Day");
                } else {
                    str = info.beginDateTime.time().toString("hh:mm");
                }
            }

            QFontMetrics fm2 = painter.fontMetrics();

            if (adjustPos) {
                painter.drawText(QRect(width() - 70 - 18, bheight, 57, labelheight - 2), Qt::AlignRight | Qt::AlignVCenter, str);
            } else {
                painter.drawText(QRect(width() - 70, bheight, 57, labelheight - 2), Qt::AlignRight | Qt::AlignVCenter, str);
            }
            painter.restore();
        }
    }
}

void CYearSchceduleView::paintItem()
{
    QFont font;
    font.setPixelSize(DDECalendar::FontSizeTwelve);
    QPainter painter(this);
    QRect fillRect = QRect(0, 0, width(), height());
    painter.setRenderHints(QPainter::HighQualityAntialiasing);
    QColor bcolor = m_bBackgroundcolor;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setBrush(QBrush(bcolor));
    painter.setPen(Qt::NoPen);
    painter.drawRect(fillRect);
    painter.restore();

    QString str;
    //左边文字
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setPen(m_bttextcolor);
    painter.setFont(font);
    QFontMetrics fm = painter.fontMetrics();
    QString tStitlename = tr("No event");

    painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, tStitlename);
    painter.restore();
}

CYearSchceduleOutView::CYearSchceduleOutView(QWidget *parent)
    : DArrowRectangle(DArrowRectangle::ArrowLeft, parent)
{
    yearschceduleview = new CYearSchceduleView ();
    this->setContent(yearschceduleview);
}

void CYearSchceduleOutView::setSoloDay(QString soloday)
{
    yearschceduleview->setSoloDay(soloday);
}

void CYearSchceduleOutView::setData(QVector<ScheduleDtailInfo> &vListData)
{
    list_count = vListData.size();
    yearschceduleview->setData(vListData);
    scheduleinfoList = yearschceduleview->getlistdate();
}

void CYearSchceduleOutView::clearData()
{
    yearschceduleview->clearData();
}

void CYearSchceduleOutView::showWindow()
{
    int w = yearschceduleview->showWindow();
    this->setFixedSize(w, yearschceduleview->height());
}

void CYearSchceduleOutView::setTheMe(int type)
{
    yearschceduleview->setTheMe(type);
}

void CYearSchceduleOutView::setDtype(int type, int arrowheight)
{
    yearschceduleview->setDtype(type, arrowheight);
}

void CYearSchceduleOutView::setCurrentDate(QDate cdate)
{
    currentdate = cdate;
    yearschceduleview->setCurrentDate(cdate);
}

void CYearSchceduleOutView::adjustPosition(bool ad)
{
    yearschceduleview->adjustPosition(ad);
}

void CYearSchceduleOutView::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    QPoint pos = QCursor::pos();
    pos = this->mapFromGlobal(pos);
    QVector<QRect> rect_press;
    QRect rect(35,50,width() - 50,20);
    int listShow = 0;

    if (!scheduleinfoList.isEmpty()) {
        if (scheduleinfoList.size() < DDEYearCalendar::YearScheduleListMaxcount)
            listShow = scheduleinfoList.size();
        else
            listShow = DDEYearCalendar::YearScheduleListMaxcount;
    }
    for (int i = 0; i < listShow; i++) {
        if (this->arrowDirection() == DArrowRectangle::ArrowLeft)
            rect_press.append(QRect(35, 20 + i * 30, width() - 50, 20));
        else
            rect_press.append(QRect(20, 20 + i * 30, width() - 50, 20));
    }
    for (int i = 0; i < listShow; i++) {
        if (rect_press.at(i).contains(pos)) {
            if (i > 3 && list_count > DDEYearCalendar::YearScheduleListMaxcount) {
                emit signalsViewSelectDate(currentdate);
                this->hide();
                //跳转到周视图
            } else {
                if (scheduleinfoList.at(i).type.ID != DDECalendar::FestivalTypeID) {
                    CSchceduleDlg dlg(0);
                    dlg.setData(scheduleinfoList.at(i));
                    if (dlg.exec() == DDialog::Accepted)
                        emit signalupdateschcedule();
                }
            }
        }
    }
}
