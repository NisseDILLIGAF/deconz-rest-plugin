/*
 * Copyright (C) 2013 dresden elektronik ingenieurtechnik gmbh.
 * All rights reserved.
 *
 * The software in this package is published under the terms of the BSD
 * style license a copy of which has been included with this distribution in
 * the LICENSE.txt file.
 *
 */

#ifndef GROUP_H
#define GROUP_H

#include <stdint.h>
#include <QString>
#include <QTime>
#include <vector>
#include "scene.h"

/*! \class Group

    Represents the group state of lights.
 */
class Group
{
public:
    enum State
    {
        StateNormal,
        StateDeleted
    };

    Group();
    uint16_t address() const;
    void setAddress(uint16_t address);
    const QString &id() const;
    const QString &name() const;
    void setName(const QString &name);
    State state() const;
    void setState(State state);
    bool isOn() const;
    void setIsOn(bool on);

    uint16_t colorX;
    uint16_t colorY;
    uint16_t hue;
    qreal hueReal;
    uint16_t sat;
    uint16_t level;
    QString etag;
    std::vector<Scene> scenes;
    QTime sendTime;

private:
    State m_state;
    uint16_t m_addr;
    QString m_id;
    QString m_name;
    bool m_on;
};

#endif // GROUP_H