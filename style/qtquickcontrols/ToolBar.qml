/* SPDX-FileCopyrightText: 2020 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import org.kde.kirigami 2.14 as Kirigami

T.ToolBar {
    id: control

    readonly property bool __isHeader: control.position === T.ToolBar.Header
    readonly property bool __isFooter: control.position === T.ToolBar.Footer

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    spacing: Kirigami.Units.smallSpacing

    padding: Kirigami.Units.smallSpacing
    topPadding: __isFooter ? 1 : 0 // Add space for the separator above the footer
    bottomPadding: __isHeader ? 1 : 0 // Add space for the separator below the header

    Kirigami.Theme.inherit: !__isHeader
    Kirigami.Theme.colorSet: Kirigami.Theme.Header

    background: Rectangle {
        // Enough height for Buttons/ComboBoxes/TextFields with smallSpacing padding on top and bottom
        implicitHeight: Kirigami.Units.mediumControlHeight + (Kirigami.Units.smallSpacing * 2) + (separator.visible ? 1 : 0) 
        color: Kirigami.Theme.backgroundColor
        Kirigami.Separator {
            id: separator
            visible: control.__isHeader || control.__isFooter
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: control.__isFooter ? parent.top : parent.bottom
            }
        }
    }
}
