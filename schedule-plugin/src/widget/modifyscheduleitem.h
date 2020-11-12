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
#ifndef MODIFYSCHEDULEITEM_H
#define MODIFYSCHEDULEITEM_H

#include "itemwidget.h"
#include <QMouseEvent>

class modifyScheduleItem : public ItemWidget
{
    Q_OBJECT
public:
    explicit modifyScheduleItem(QWidget *parent = nullptr);
    int getIndex() const;
    void setIndex(int Index);

protected:
    QColor LineColor() const;
    void setLineColor(const QColor &LineColor);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setTheMe(const int type) override;
    void DrawItem(QPainter &painter) override;
    void drawIndex(QPainter &painter);
    void drawLine(QPainter &painter);
    void drawTitleContent(QPainter &painter);
    void drawScheduleColorLine(QPainter &painter);
    void drawTime(QPainter &painter);
    void drawDate(QPainter &painter);
signals:
    void signalSendSelectScheduleIndex(int index);
public slots:
private:
    int m_Index {1};
    const int m_IndexWidth {16};
    const int m_IndexX {14};
    QColor m_LineColor {"#000000"};
    const int m_TitleX {51};
    const int m_TItleRightMargin {97};
    const int m_DateTimeOffset = 3;
};

#endif // MODIFYSCHEDULEITEM_H
