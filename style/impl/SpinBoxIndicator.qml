/* SPDX-FileCopyrightText: 2020 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

import QtQuick 2.15
import org.kde.kirigami 2.14 as Kirigami

Item {
    id: root

    property QtObject button // QQuickSpinButton is a QObject
    property bool mirrored: false
    property int alignment: Qt.AlignLeft
    property bool leftAligned: root.alignment == Qt.AlignLeft
    property bool rightAligned: root.alignment == Qt.AlignRight
    property real leftRadius: {
        if ((leftAligned && !mirrored)
            || (rightAligned && mirrored)) {
            return Kirigami.Units.smallRadius
        } else {
            return 0
        }
    }
    property real rightRadius: {
        if ((rightAligned && !mirrored)
            || (leftAligned && mirrored)) {
            return Kirigami.Units.smallRadius
        } else {
            return 0
        }
    }

    x: {
        if ((leftAligned && !mirrored)
            || (rightAligned && mirrored)) {
            return 0
        } else {
            return parent.width - width
        }
    }
    height: parent.height

    implicitWidth: implicitHeight
    implicitHeight: Kirigami.Units.mediumControlHeight

    Rectangle {
        width: Kirigami.Units.smallBorder
        x: {
            if ((leftAligned && !mirrored)
                || (rightAligned && mirrored)) {
                return parent.width - width
            } else {
                return 0
            }
        }
        anchors {
            top: parent.top
            bottom: parent.bottom
            topMargin: Kirigami.Units.smallSpacing
            bottomMargin: Kirigami.Units.smallSpacing
        }
        color: {
            if (button.pressed || button.hovered) {
                return Kirigami.Theme.focusColor
            } else {
                return Kirigami.Theme.separatorColor
            }
        }
    }

    Kirigami.ShadowedRectangle {
        id: pressedBg
        Kirigami.Theme.colorSet: Kirigami.Theme.Button
        Kirigami.Theme.inherit: false
        visible: button.pressed
        anchors.fill: parent
        color: Kirigami.Theme.alternateBackgroundColor
        corners {
            topLeftRadius: root.leftRadius
            topRightRadius: root.rightRadius
            bottomLeftRadius: root.leftRadius
            bottomRightRadius: root.rightRadius
        }
        border.color: Kirigami.Theme.focusColor
        border.width: Kirigami.Units.smallBorder
    }

    Kirigami.Icon {
        implicitHeight: Kirigami.Units.iconSizes.auto
        implicitWidth: implicitHeight
        anchors {
            centerIn: parent
        }
        source: {
            // For some reason I don't need to use the fancier logic with this
            if (leftAligned) {
                return "arrow-down"
            } else {
                return "arrow-up"
            }
        }
    }
}
