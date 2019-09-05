/********************************************************************
Copyright © 2019 Roman Gilg <subdiff@gmail.com>
Copyright © 2012 Dan Vratil <dvratil@redhat.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
import QtQuick 2.12
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as Controls
import QtGraphicalEffects 1.0

Rectangle {
    id: output

    property bool isSelected: root.selectedOutput === model.index

    onIsSelectedChanged: {
        if (isSelected) {
            z = 89;
        } else {
            z = 0;
        }
    }

    function getAbsolutePosition(pos) {
        return Qt.point((pos.x - screen.xOffset) * screen.relativeFactor,
                        (pos.y - screen.yOffset) * screen.relativeFactor) ;
    }

    visible: model.enabled
    onVisibleChanged: screen.resetTotalSize()

    x: model.position.x / screen.relativeFactor + screen.xOffset
    y: model.position.y / screen.relativeFactor + screen.yOffset

    width: model.size.width / screen.relativeFactor
    height: model.size.height / screen.relativeFactor

    SystemPalette {
        id: palette
    }

    radius: 4
    color: palette.window
    smooth: true
    clip: true

    border {
        color: isSelected ? palette.highlight : palette.shadow
        width: 1

        Behavior on color {
            PropertyAnimation {
                duration: 150
            }
        }
    }

    Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.centerIn: parent
            spacing: units.smallSpacing
            width: parent.width
            Layout.maximumHeight: parent.height

            Controls.Label {
                Layout.fillWidth: true
                Layout.margins: units.smallSpacing

                text: model.display
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                color: palette.text
            }

            Controls.Label {
                Layout.fillWidth: true
                Layout.margins: units.smallSpacing

                text: "(" + model.size.width + "x" + model.size.height + ")"
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                color: palette.text
            }
        }
    }

    Rectangle {
        id: posLabel

        y: 4
        x: 4
        width: childrenRect.width + 5
        height: childrenRect.height + 2
        radius: 4

        opacity: model.enabled &&
                 (tapHandler.isLongPressed || dragHandler.active) ? 0.9 : 0.0


        color: palette.shadow

        Text {
            id: posLabelText

            y: 2
            x: 2

            text: model.normalizedPosition.x + "," + model.normalizedPosition.y
            color: "white"
        }

        Behavior on opacity {
            PropertyAnimation {
                duration: 100;
            }
        }
    }

    Item {
        id: orientationPanelContainer

        anchors.fill: output
        visible: false

        Rectangle {
            id: orientationPanel
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            height: 10
            color: isSelected ? palette.highlight : palette.shadow
            smooth: true

            Behavior on color {
                PropertyAnimation {
                    duration: 150
                }
            }
        }
    }

    states: [
        State {
            name: "rot90"
            when: model.rotation === 2
            PropertyChanges {
                target: orientationPanel
                height: undefined
                width: 10
            }
            AnchorChanges {
                target: orientationPanel
                anchors.right: undefined
                anchors.top: orientationPanelContainer.top
            }
        },
        State {
            name: "rot180"
            when: model.rotation === 4
            AnchorChanges {
                target: orientationPanel
                anchors.top: orientationPanelContainer.top
                anchors.bottom: undefined
            }
        },
        State {
            name: "rot270"
            when: model.rotation === 8
            PropertyChanges {
                target: orientationPanel
                height: undefined
                width: 10
            }
            AnchorChanges {
                target: orientationPanel
                anchors.left: undefined
                anchors.top: orientationPanelContainer.top
            }
        }
    ]

    OpacityMask {
        anchors.fill: orientationPanelContainer
        source: orientationPanelContainer
        maskSource: output
    }

    property point dragStartPosition

    TapHandler {
        id: tapHandler
        property bool isLongPressed: false

        function bindSelectedOutput() {
            root.selectedOutput
                    = Qt.binding(function() { return model.index; });
        }
        onPressedChanged: {
            if (pressed) {
                bindSelectedOutput();
                dragStartPosition = Qt.point(output.x, output.y)
            } else {
                isLongPressed = false;
            }
        }
        onLongPressed: isLongPressed = true;
        longPressThreshold: 0.3
    }
    DragHandler {
        id: dragHandler
        acceptedButtons: Qt.LeftButton
        target: null

        onTranslationChanged: {
            var newX = dragStartPosition.x + translation.x;
            var newY = dragStartPosition.y + translation.y;
            model.position = getAbsolutePosition(Qt.point(newX, newY));
        }
    }
}

