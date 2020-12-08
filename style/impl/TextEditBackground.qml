/* SPDX-FileCopyrightText: 2020 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import org.kde.kirigami 2.14 as Kirigami

Rectangle {
    id: mainBackground

    property alias control: mainBackground.parent

    implicitWidth: implicitHeight
    implicitHeight: Kirigami.Units.mediumControlHeight

    color: Kirigami.Theme.backgroundColor
    radius: Kirigami.Units.smallRadius
    Rectangle {
        Kirigami.Theme.colorSet: Kirigami.Theme.Button
        Kirigami.Theme.inherit: false
        radius: parent.radius
        anchors.fill: parent
        color: "transparent"
        border {
            color: control.activeFocus || control.hovered ?
                    Kirigami.Theme.focusColor : Kirigami.Theme.separatorColor
            width: Kirigami.Units.smallBorder
        }
    }
}
