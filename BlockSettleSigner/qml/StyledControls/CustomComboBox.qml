import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Controls.Styles 1.4
import "../BsStyles"

ComboBox {
    id: control
    model: ["First", "Second", "Third"]
    spacing: 3

    contentItem: Text {
        leftPadding: 7
        rightPadding: control.indicator.width + control.spacing
        color: BSStyle.comboBoxItemTextHighlightedColor
        text: control.displayText
        font: control.font

        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }


    indicator: Canvas {
        id: canvas
        x: control.width - width / 2 - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 16
        height: 8
        contextType: "2d"

        Connections {
            target: control
            onPressedChanged: canvas.requestPaint()
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(8,8);
            context.lineTo(16, 0);
            context.lineTo(15, 0);
            context.lineTo(8,7);
            context.lineTo(1, 0);
            context.closePath();
            context.fillStyle = BSStyle.comboBoxItemTextHighlightedColor;
            context.fill();
        }
    }

    background: Rectangle {
        implicitWidth: 120
        color: BSStyle.comboBoxBgColor
        implicitHeight: 40
        border.color: BSStyle.inputsBorderColor
        border.width: control.visualFocus ? 2 : 1
        radius: 2
    }

    delegate: ItemDelegate {
        width: control.width
        id: menuItem

        contentItem: Text {
            text: modelData
            color: menuItem.highlighted ? BSStyle.comboBoxItemTextColor : BSStyle.comboBoxItemTextHighlightedColor
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: control.highlightedIndex === index

        background: Rectangle {
            color: menuItem.highlighted ? BSStyle.comboBoxItemBgHighlightedColor : BSStyle.comboBoxItemBgColor
        }
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }


        background: Rectangle {
            color: BSStyle.comboBoxItemBgColor
            border.color: BSStyle.inputsBorderColor
            radius: 0
        }
    }
}

