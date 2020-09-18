/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     uniontech  <uniontech@uniontech.com>
*
* Maintainer: uniontech  <chenhaifeng@uniontech.com>
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
#include "monthgraphiview.h"
#include "../widget/monthWidget/monthschceduleview.h"
#include "../dialog/schceduledlg.h"
#include "../dialog/schcedulectrldlg.h"
#include "../dialog/myschceduleview.h"
#include "constants.h"

#include <DHiDPIHelper>

#include <QAction>
#include <QMimeData>
#include <QShortcut>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainterPath>

bool CDayGraphicsItem::m_LunarVisible = false;

CMonthGraphiview::CMonthGraphiview(QWidget *parent)
    : DragInfoGraphicsView (parent)
{
    QShortcut *shortcut = new QShortcut(parent);
    shortcut->setKey(QKeySequence(QLatin1String("Delete")));
    connect(shortcut,
            &QShortcut::activated, this,
            &CMonthGraphiview::slotDeleteItem);

    m_MonthSchceduleView = new CMonthSchceduleView(this,m_Scene);
    connect(this,&CMonthGraphiview::signalFontChange,
            m_MonthSchceduleView,&CMonthSchceduleView::slotFontChange);

    for (int i = 0; i < DDEMonthCalendar::ItemSizeofMonthDay; ++i) {
        CDayGraphicsItem *item = new CDayGraphicsItem();
        item->setZValue(-1);
        m_DayItem.append(item);
        m_Scene->addItem(item);
        item->setitemnum(i);
    }
    updateSize();
}

CMonthGraphiview::~CMonthGraphiview()
{
    delete m_MonthSchceduleView;

    for (int i = 0; i < m_DayItem.size(); ++i) {
        CDayGraphicsItem *item = m_DayItem.at(i);
        delete item;
        item = nullptr;
    }

    m_DayItem.clear();
}

void CMonthGraphiview::setTheMe(int type)
{
    m_themetype = type;

    for (int i = 0; i < m_DayItem.size(); ++i) {
        m_DayItem.at(i)->setTheMe(type);
    }

    m_MonthSchceduleView->setTheMe(type);
}

void CMonthGraphiview::setDate(const QDate date[42])
{
    if (date[0].day() !=1) {
        m_currentMonth = date[0].addMonths(1).month();
    } else {
        m_currentMonth = date[0].month();
    }

    for (int i = 0; i < m_DayItem.size(); ++i) {
        m_DayItem.at(i)->setData(date[i]);
        m_DayItem.at(i)->setCurrentMonth(date[i].month() == m_currentMonth);
        m_DayItem.at(i)->setStatus(CDayGraphicsItem::HolidayStatus(getFestivalInfoByDate(date[i])));
    }

    this->scene()->update();
}

void CMonthGraphiview::setLunarInfo(QMap<QDate, CaLunarDayInfo> *lunarCache)
{
    m_lunarCache = lunarCache;
    updateLunar();

}

void CMonthGraphiview::setLunarVisible(bool visible)
{
    CDayGraphicsItem::m_LunarVisible = visible;
}

void CMonthGraphiview::setScheduleInfo(const QVector<ScheduleDateRangeInfo> &info)
{
    m_shceludelistdata = info;
    updateInfo();
}

void CMonthGraphiview::setSelectSchedule(const ScheduleDtailInfo &scheduleInfo)
{
    QVector<QGraphicsRectItem *> mscheduleShowBtn = m_MonthSchceduleView->getScheduleShowItem();

    for (int i = 0; i < mscheduleShowBtn.size(); ++i) {
        CMonthSchceduleWidgetItem *item = dynamic_cast<CMonthSchceduleWidgetItem *>(mscheduleShowBtn.at(i));

        if (item == nullptr) continue;

        if (scheduleInfo == item->getData()) {
            item->setStartValue(0);
            item->setEndValue(4);
            item->startAnimation();
        }
    }
}

void CMonthGraphiview::updateSize()
{
    m_Scene->setSceneRect(this->viewport()->rect());
    qreal w = m_Scene->width() / DDEMonthCalendar::AFewDaysofWeek;
    qreal h = m_Scene->height() / DDEMonthCalendar::LinesNumofMonth;
    QRectF rect ;
    int w_offset = 0;
    int h_offset = 0;

    for (int i = 0 ; i < m_DayItem.size(); ++i) {
        h_offset = i / DDEMonthCalendar::AFewDaysofWeek;
        w_offset = i % DDEMonthCalendar::AFewDaysofWeek;
        rect.setRect(w*w_offset,
                     h*h_offset,
                     w,
                     h);
        m_DayItem.at(i)->setRect(rect);
    }
}

void CMonthGraphiview::updateLunar()
{
    QDate date;
    CaLunarDayInfo info;
    QString lunarStr("");

    for (int i = 0 ; i < m_DayItem.size(); ++i) {
        date = m_DayItem.at(i)->getDate();
        if (m_lunarCache->contains(date)) {
            info = m_lunarCache->value(date);

            if (info.mLunarDayName == "初一") {
                info.mLunarDayName = info.mLunarMonthName + info.mLunarDayName;
            }

            if (info.mTerm.isEmpty()) {
                lunarStr = info.mLunarDayName;
            } else {
                lunarStr = info.mTerm;
            }
        } else {
            lunarStr = "";
        }
        m_DayItem.at(i)->setLunar(lunarStr);
    }
}

void CMonthGraphiview::updateInfo()
{
    int h = m_MonthSchceduleView->getSchceduleHeight();
    m_MonthSchceduleView->setallsize(this->viewport()->width(),
                                     this->viewport()->height(),
                                     0, 0, 0, h);
    m_MonthSchceduleView->setData(m_shceludelistdata, 1);

    switch (m_DragStatus) {
    case IsCreate:
        upDateInfoShow(IsCreate,m_DragScheduleInfo);
        break;
    case ChangeWhole:
        upDateInfoShow(ChangeWhole,m_DragScheduleInfo);
        break;
    default:

        break;
    }
    update();
}

char CMonthGraphiview::getFestivalInfoByDate(const QDate &date)
{
    for (int i = 0; i < m_festivallist.count(); i++) {
        for (int j = 0; j < m_festivallist[i].listHoliday.count(); j++) {
            if (m_festivallist[i].listHoliday[j].date == date) {
                return m_festivallist[i].listHoliday[j].status;
            }
        }
    }
    return 0;
}

QPointF CMonthGraphiview::getItemPos(const QPoint &p, const QRectF &itemRect)
{
    QPointF scenePos = this->mapToScene(p);
    return  QPointF(scenePos.x()-itemRect.x(),
                    scenePos.y()-itemRect.y());
}

CMonthGraphiview::PosInItem CMonthGraphiview::getPosInItem(const QPoint &p, const QRectF &itemRect)
{
    QPointF scenePos = this->mapToScene(p);
    QPointF itemPos = QPointF(scenePos.x()-itemRect.x(),
                              scenePos.y()-itemRect.y());
    qreal bottomy = itemRect.width()- itemPos.x();

    if (itemPos.x()<5) {
        return LEFT;
    }

    if (bottomy <5) {
        return RIGHT;
    }

    return MIDDLE;
}

QDateTime CMonthGraphiview::getPosDate(const QPoint &p)
{
    if (!this->sceneRect().contains(p))
        return m_MoveDate;
    QRectF rect = this->sceneRect();
    qreal x =0;
    qreal y = 0;

    if (p.x()<0) {
        x =0;
    } else if (p.x()>(rect.width()-10)) {
        x = rect.width()-10;
    } else {
        x = p.x();
    }

    if (p.y()<0) {
        y =0;
    } else if (p.y()>(rect.height()-10)) {
        y = rect.height()-10;
    } else {
        y = p.y();
    }

    int xoffset = qFloor(x / (rect.width() / DDEMonthCalendar::AFewDaysofWeek)) % DDEMonthCalendar::AFewDaysofWeek;
    int yoffset = qFloor(y / (rect.height() / DDEMonthCalendar::LinesNumofMonth)) % DDEMonthCalendar::LinesNumofMonth;

    return  QDateTime(m_DayItem[xoffset+yoffset*7]->getDate(),
                      QTime(0,0,0));
}

void CMonthGraphiview::upDateInfoShow(const CMonthGraphiview::DragStatus &status, const ScheduleDtailInfo &info)
{
    switch (status) {
    case NONE:
        Q_UNUSED(info);
        break;
    case ChangeBegin:
    case ChangeEnd: {
        m_MonthSchceduleView->changeDate(info);
    }
    break;
    case ChangeWhole: {
    }
    break;
    case IsCreate:
        m_MonthSchceduleView->updateDate(info);
        break;
    }
}

void CMonthGraphiview::updateScheduleInfo(const ScheduleDtailInfo &info)
{
    if (info.rpeat >0) {
        CSchceduleDlg::ChangeRecurInfo(this,info,
                                       m_PressScheduleInfo,m_themetype);
    } else {
        CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->updateScheduleInfo(
            info);
    }
}

void CMonthGraphiview::DeleteItem(const ScheduleDtailInfo &info)
{
    emit signalViewtransparentFrame(1);

    if (info.rpeat == 0) {
        CSchceduleCtrlDlg msgBox(this);
        msgBox.setText(tr("You are deleting an event."));
        msgBox.setInformativeText(tr("Are you sure you want to delete this event?"));
        msgBox.addPushButton(tr("Cancel"), true);
        msgBox.addWaringButton(tr("Delete"), true);
        msgBox.exec();

        if (msgBox.clickButton() == 0) {
            emit signalViewtransparentFrame(0);
            return;
        } else if (msgBox.clickButton() == 1) {
            CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->deleteScheduleInfoById(info.id);
        }
    } else {
        if (info.RecurID == 0) {
            CSchceduleCtrlDlg msgBox(this);
            msgBox.setText(tr("You are deleting an event."));
            msgBox.setInformativeText(tr("Do you want to delete all occurrences of this event, or only the selected occurrence?"));
            msgBox.addPushButton(tr("Cancel"));
            msgBox.addPushButton(tr("Delete All"));
            msgBox.addWaringButton(tr("Delete Only This Event"));
            msgBox.exec();

            if (msgBox.clickButton() == 0) {
                emit signalViewtransparentFrame(0);
                return;
            } else if (msgBox.clickButton() == 1) {
                CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->deleteScheduleInfoById(info.id);
            } else if (msgBox.clickButton() == 2) {

                ScheduleDtailInfo newschedule;
                CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->getScheduleInfoById(info.id, newschedule);
                newschedule.ignore.append(info.beginDateTime);
                CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->updateScheduleInfo(newschedule);
            }
        } else {
            CSchceduleCtrlDlg msgBox(this);
            msgBox.setText(tr("You are deleting an event."));
            msgBox.setInformativeText(tr("Do you want to delete this and all future occurrences of this event, or only the selected occurrence?"));
            msgBox.addPushButton(tr("Cancel"));
            msgBox.addPushButton(tr("Delete All Future Events"));
            msgBox.addWaringButton(tr("Delete Only This Event"));
            msgBox.exec();

            if (msgBox.clickButton() == 0) {
                emit signalViewtransparentFrame(0);
                return;
            } else if (msgBox.clickButton() == 1) {
                ScheduleDtailInfo newschedule;
                CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->getScheduleInfoById(info.id, newschedule);
                newschedule.enddata.type = 2;
                newschedule.enddata.date = info.beginDateTime.addDays(-1);
                CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->updateScheduleInfo(newschedule);

            } else if (msgBox.clickButton() == 2) {
                ScheduleDtailInfo newschedule;
                CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->getScheduleInfoById(info.id, newschedule);
                newschedule.ignore.append(info.beginDateTime);
                CScheduleDataManage::getScheduleDataManage()->getscheduleDataCtrl()->updateScheduleInfo(newschedule);
            }
        }
    }
    emit signalsUpdateShcedule();
    emit signalViewtransparentFrame(0);
}

void CMonthGraphiview::setPressSelectInfo(const ScheduleDtailInfo &info)
{
    CScheduleDataManage::getScheduleDataManage()->setPressSelectInfo(info);
}

void CMonthGraphiview::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    QGraphicsItem *listItem =itemAt(event->pos());
    CMonthSchceduleNumButton *item = dynamic_cast<CMonthSchceduleNumButton *>(listItem);

    if (item!= nullptr) {
        //双击切换视图
        if (item->getDate().year() > DDECalendar::QueryEarliestYear) {
            emit signalsViewSelectDate(item->getDate());
        }
        return;
    }

    CMonthSchceduleWidgetItem *infoitem = dynamic_cast<CMonthSchceduleWidgetItem *>(listItem);

    if (infoitem != nullptr) {
        CMySchceduleView dlg(infoitem->getData(), this);
        connect(&dlg, &CMySchceduleView::signalsEditorDelete, this, &CMonthGraphiview::signalsUpdateShcedule);
        connect(&dlg, &CMySchceduleView::signalViewtransparentFrame,
                this, &CMonthGraphiview::signalViewtransparentFrame);
        dlg.exec();
        return;
    }

    CDayGraphicsItem *Dayitem = dynamic_cast<CDayGraphicsItem *>(listItem);

    if (Dayitem !=nullptr) {
        QPointF point = getItemPos(event->pos(),Dayitem->rect());
        if (point.y()<38) {
            //双击切换视图
            if (Dayitem->getDate().year() > DDECalendar::QueryEarliestYear) {
                emit signalsViewSelectDate(Dayitem->getDate());
            }
        } else {
            //双击新建日程
            slotCreate(QDateTime(Dayitem->getDate(),QTime(0,0,0)));
        }
    }
}

void CMonthGraphiview::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateSize();
    updateInfo();
}
void CMonthGraphiview::changeEvent(QEvent *event)
{
    if (event->type() ==QEvent::FontChange) {
        emit signalFontChange();
    }
}

void CMonthGraphiview::wheelEvent(QWheelEvent *e)
{
    Q_UNUSED(e);
}

void CMonthGraphiview::setDragPixmap(QDrag *drag, DragInfoItem *item)
{
    CMonthSchceduleWidgetItem *infoitem = dynamic_cast<CMonthSchceduleWidgetItem *>(item);
    drag->setPixmap(infoitem->getPixmap());
}

bool CMonthGraphiview::MeetCreationConditions(const QDateTime &date)
{
    return  qAbs(date.daysTo(m_PressDate)<43);
}

bool CMonthGraphiview::IsEqualtime(const QDateTime &timeFirst, const QDateTime &timeSecond)
{
    return timeFirst.date()==timeSecond.date();
}

bool CMonthGraphiview::JudgeIsCreate(const QPointF &pos)
{
    return qAbs(pos.x()-m_PressPos.x())>20 ||qAbs(m_PressDate.daysTo(getPosDate(pos.toPoint())))>0;
}

void CMonthGraphiview::RightClickToCreate(QGraphicsItem *listItem,const QPoint &pos)
{
    Q_UNUSED(pos);
    CDayGraphicsItem *Dayitem = dynamic_cast<CDayGraphicsItem *>(listItem);

    if (Dayitem != nullptr) {
        m_rightMenu->clear();
        m_rightMenu->addAction(m_createAction);
        m_createDate.setDate(Dayitem->getDate());
        m_rightMenu->exec(QCursor::pos());
    }
}

void CMonthGraphiview::MoveInfoProcess(ScheduleDtailInfo &info, const QPointF &pos)
{
    qint64 offset       = m_PressDate.daysTo(m_MoveDate);
    info.beginDateTime  = info.beginDateTime.addDays(offset);
    info.endDateTime    = info.endDateTime.addDays(offset);
    qreal y = 0;
    QRectF rect = this->sceneRect();

    if (pos.y()<0) {
        y =0;
    } else if (pos.y()>rect.height()) {
        y = rect.height();
    } else {
        y = pos.y();
    }

    int yoffset = qFloor(y / (rect.height() / DDEMonthCalendar::LinesNumofMonth)) % DDEMonthCalendar::LinesNumofMonth;
    info.IsMoveInfo = true;
    m_MonthSchceduleView->updateDate(yoffset,info);
}

QDateTime CMonthGraphiview::getDragScheduleInfoBeginTime(const QDateTime &moveDateTime)
{
    return moveDateTime.daysTo(m_InfoEndTime)<0 ?
           QDateTime(m_InfoEndTime.date(),m_InfoBeginTime.time()):
           QDateTime(moveDateTime.date(),m_InfoBeginTime.time());
}

QDateTime CMonthGraphiview::getDragScheduleInfoEndTime(const QDateTime &moveDateTime)
{
    return m_InfoBeginTime.daysTo(moveDateTime)<0 ?
           QDateTime(m_InfoBeginTime.date(),m_InfoEndTime.time()):
           QDateTime(moveDateTime.date(),m_InfoEndTime.time());
}

void CMonthGraphiview::slotCreate(const QDateTime &date)
{
    emit signalViewtransparentFrame(1);
    CSchceduleDlg dlg(1, this);
    QDateTime tDatatime;
    tDatatime.setDate(date.date());

    if (date.date() == QDate::currentDate()) {
        tDatatime.setTime(QTime::currentTime());
    } else {
        tDatatime.setTime(QTime(8, 0));
    }

    dlg.setDate(tDatatime);
    dlg.setAllDay(true);

    if (dlg.exec() == DDialog::Accepted) {
        emit signalsUpdateShcedule();
        emit signalsSchceduleUpdate(0);
    }
    emit signalViewtransparentFrame(0);
}


void CMonthGraphiview::slotdelete(const int id)
{
    Q_UNUSED(id);
    emit signalsSchceduleUpdate(0);
}

void CMonthGraphiview::slotDeleteItem()
{
    if (CScheduleDataManage::getScheduleDataManage()->getPressSelectInfo().type.ID <0) {
        return;
    }

    if (CScheduleDataManage::getScheduleDataManage()->getPressSelectInfo().type.ID != DDECalendar::FestivalTypeID) {
        DeleteItem(CScheduleDataManage::getScheduleDataManage()->getPressSelectInfo());
    }

    CScheduleDataManage::getScheduleDataManage()->setPressSelectInfo(ScheduleDtailInfo());
}

CDayGraphicsItem::CDayGraphicsItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
    , m_Date(QDate::currentDate())
    , m_DayLunar("")
    , m_DayStatus(H_NONE)
{
    m_dayNumFont.setPixelSize(DDECalendar::FontSizeTwentyfour);
    m_dayNumFont.setWeight(QFont::Light);

    m_LunerFont.setPixelSize(DDECalendar::FontSizeTwelve);
    m_LunerFont.setWeight(QFont::Normal);
}

CDayGraphicsItem::~CDayGraphicsItem()
{

}

void CDayGraphicsItem::setData(const QDate &date)
{
    m_Date = date;
    setLunar("");
}

void CDayGraphicsItem::setLunar(const QString &lunar)
{
    m_DayLunar = lunar;
}

void CDayGraphicsItem::setStatus(const CDayGraphicsItem::HolidayStatus &status)
{
    m_DayStatus = status;
}

void CDayGraphicsItem::setTheMe(int type)
{
    m_themetype = type;

    if (type == 0 || type == 1) {
        m_dayNumColor = "#000000";
        m_dayNumCurrentColor = "#FFFFFF";

        m_LunerColor = "#5E5E5E";
        m_LunerColor.setAlphaF(0.5);

        m_fillColor = Qt::white;
        m_banColor = "#FF7171";
        m_banColor.setAlphaF(0.1);
        m_xiuColor = "#ADFF71";
        m_xiuColor.setAlphaF(0.1);

        m_BorderColor = "#000000";
        m_BorderColor.setAlphaF(0.05);
    } else if (type == 2) {
        m_dayNumColor = "#C0C6D4";
        m_dayNumCurrentColor = "#B8D3FF";

        m_LunerColor = "#ABDAFF";
        m_LunerColor.setAlphaF(0.5);

        m_fillColor = "#000000";
        m_fillColor.setAlphaF(0.05);
        m_banColor = "#FF7171";
        m_banColor.setAlphaF(0.1);
        m_xiuColor = "#ADFF71";
        m_xiuColor.setAlphaF(0.1);

        m_BorderColor = "#000000";
        m_BorderColor.setAlphaF(0.05);
    }
    update();
}

void CDayGraphicsItem::setitemnum(int num)
{
    m_itemnum = num;
}

void CDayGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    const int hh = 36;
    m_currentColor = CScheduleDataManage::getScheduleDataManage()->getSystemActiveColor();
    painter->setRenderHints(QPainter::Antialiasing);
    //绘制背景
    if (m_LunarVisible) {
        switch (m_DayStatus) {
        case H_WORK:
            painter->setBrush(QBrush(m_banColor));
            break;
        case H_REST:
            painter->setBrush(QBrush(m_xiuColor));
            break;
        default:
            painter->setBrush(QBrush(m_fillColor));
            break;
        }
    } else {
        painter->setBrush(QBrush(m_fillColor));
    }

    if (!m_IsCurrentMonth) {
        painter->setOpacity(0.4);
    }
    QPen pen;
    pen.setWidth(1);
    pen.setColor(m_BorderColor);
    painter->setPen(pen);

    if (m_itemnum == 35) {
        QPainterPath painterpath;
        painterpath.moveTo(0, this->rect().y());
        painterpath.lineTo(0, this->rect().y() + this->rect().height() - m_radius);
        painterpath.arcTo(QRectF(0,
                                 this->rect().y() + this->rect().height() - m_radius * 2,
                                 m_radius * 2,
                                 m_radius * 2),
                          180, 90);
        painterpath.lineTo(this->rect().width(), this->rect().y() + this->rect().height());
        painterpath.lineTo(this->rect().width(), this->rect().y());
        painterpath.lineTo(0, this->rect().y());
        painter->drawPath(painterpath);
    } else if (m_itemnum == 41) {
        QPainterPath painterpath;
        painterpath.moveTo(this->rect().x(), this->rect().y());
        painterpath.lineTo(this->rect().x(), this->rect().y() + this->rect().height());
        painterpath.lineTo(this->rect().x() + this->rect().width() - m_radius, this->rect().y() + this->rect().height());
        painterpath.arcTo(QRectF(this->rect().x() + this->rect().width() - m_radius * 2,
                                 this->rect().y() + this->rect().height() - m_radius * 2,
                                 m_radius * 2,
                                 m_radius * 2),
                          270, 90);
        painterpath.lineTo(this->rect().x() + this->rect().width(), this->rect().y());
        painterpath.lineTo(this->rect().y() + this->rect().height(), this->rect().y());
        painter->drawPath(painterpath);
    } else {
        painter->drawRect(this->rect());
    }

    painter->save();
    painter->restore();
    //绘制日期
    painter->setFont(m_dayNumFont);
    QRectF fillRect;

    if (m_LunarVisible) {
        fillRect.setRect(this->rect().x()+3,this->rect().y()+4, hh, hh);
    } else {
        fillRect.setRect(this->rect().x(),this->rect().y()+4, this->rect().width(), hh);
    }

    if (m_Date ==QDate::currentDate()) {
        QFont tfont = m_dayNumFont;
        tfont.setPixelSize(DDECalendar::FontSizeTwenty);
        painter->setFont(tfont);
        painter->setPen(m_dayNumCurrentColor);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(QBrush(m_currentColor));
        painter->setPen(Qt::NoPen);
        if (m_LunarVisible)
            painter->drawEllipse(QRectF(this->rect().x() + 6, this->rect().y() + 4, hh - 8, hh - 8));
        else
            painter->drawEllipse(QRectF((this->rect().width() - hh + 8) / 2 + this->rect().x(), this->rect().y() + 4, hh - 8, hh - 8));
        painter->restore();
    } else {
        painter->setPen(m_dayNumColor);
    }

    fillRect.setY(fillRect.y()-10);
    fillRect.setX(fillRect.x()-1);
    painter->drawText(fillRect,
                      Qt::AlignCenter,
                      QString::number(m_Date.day()));

    painter->save();
    painter->restore();
    //绘制农历
    if (m_LunarVisible) {
        QFontMetrics metrics(m_LunerFont);
        int Lunarwidth = metrics.width(m_DayLunar);
        qreal filleRectX = this->rect().width()-12-3-(58 +Lunarwidth)/2;
        QRectF fillRect(this->rect().x()+filleRectX,
                        this->rect().y()+9,
                        12,
                        12);

        if (filleRectX>hh) {
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::HighQualityAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            switch (m_DayStatus) {
            case H_WORK: {
                QPixmap  pixmap = DHiDPIHelper::loadNxPixmap(":/resources/icon/ban.svg");
                painter->drawPixmap(fillRect.toRect(), pixmap);
            }
            break;
            case H_REST: {
                QPixmap pixmap = DHiDPIHelper::loadNxPixmap(":/resources/icon/xiu.svg");
                painter->drawPixmap(fillRect.toRect(), pixmap);
            }
            break;
            default:
                break;
            }
        }
        painter->setFont(m_LunerFont);
        painter->setPen(m_LunerColor);
        painter->drawText(QRectF(this->rect().x()+this->rect().width() - 58,
                                 this->rect().y()+6, 58, 18), Qt::AlignCenter, m_DayLunar);
    }
}
