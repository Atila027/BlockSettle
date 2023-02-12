
/*

***********************************************************************************
* Copyright (C) 2023, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2.15
import QtQuick.Controls 2.15

import "../BsStyles"

TableView {
    id: component
    width: 1200
    height: 200

    columnSpacing: 0
    rowSpacing: 0
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ScrollBar.vertical: ScrollBar { }

    signal copyRequested(var id)
    signal deleteRequested(int id)
    signal cellClicked(int row, int column, var data)
    signal cellDoubleClicked(int row, int column, var data)

    property int text_header_size: 11    
    property int cell_text_size: 12
    property int copy_button_column_index: 0
    property int delete_button_column_index: -1

    property int left_first_header_padding: 10
    property int left_text_padding: 10

    property var columnWidths:  ({})
    columnWidthProvider: function (column) {
        return columnWidths[column] * component.width
    }

    property int selected_row_index: -1

    onWidthChanged: component.forceLayout()

    delegate: Rectangle {
        implicitHeight: 34
        color: row === 0 ? BSStyle.tableCellBackgroundColor : (row === selected_row_index ? BSStyle.tableCellSelectedBackgroundColor : BSStyle.tableCellBackgroundColor)

        MouseArea {
            anchors.fill: parent
            preventStealing: true
            propagateComposedEvents: true
            hoverEnabled: true

            onEntered: {
                if (row !== 0) {
                    component.selected_row_index = row
                }
            }

            onExited: {
                if (row !== 0) {
                    component.selected_row_index = -1
                }
            }

            onClicked: {
                if (row !== 0) {
                    component.cellClicked(row, column, tableData)
                }
            }
            onDoubleClicked: {
                if (row !== 0) {
                    omponent.cellDoubleClicked(row, column, tableData)
                }
            } 
        }

        Row {
            width: parent.width
            height: parent.height

            Text {
                id: internal_text

                visible: column !== component.delete_button_column_index

                text: tableData
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                clip: true

                color: dataColor
                font.family: "Roboto"
                font.weight: Font.Normal
                font.pixelSize: row === 0 ? component.text_header_size : component.cell_text_size

                leftPadding: (row === 0 && column === 0)
                             ? left_first_header_padding : left_text_padding
            }

            DeleteIconButton {
                id: delete_icon
                x: 0
                visible: column === component.delete_button_column_index && row > 0
                onDeleteRequested: component.deleteRequested(row)
            }

            CopyIconButton {
                id: copy_icon
                x: internal_text.contentWidth + copy_icon.width / 2
                visible: column === component.copy_button_column_index && row == selected_row_index
                onCopy: component.copyRequested(tableData)
            }
        }

        Rectangle {
            height: 1
            width: parent.width
            color: BSStyle.tableSeparatorColor

            anchors.bottom: parent.bottom
        }
    }
}
