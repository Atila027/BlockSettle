
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

    reuseItems: false

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

    property int left_text_padding: 10

    property var columnWidths:  ({})
    columnWidthProvider: function (column) {
        return columnWidths[column] * component.width
    }

    property int selected_row_index: -1

    onWidthChanged: component.forceLayout()

    delegate: Rectangle {
        id: delega

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
                    component.cellDoubleClicked(row, column, tableData)
                }
            } 
        }

        Loader {
            id: row_loader

            width: parent.width
            height: childrenRect.height

            property int delete_button_column_index: component.delete_button_column_index
            property int text_header_size: component.text_header_size
            property int cell_text_size: component.cell_text_size
            property int copy_button_column_index: component.copy_button_column_index
            property int selected_row_index: component.selected_row_index
            property int model_row: row
            property int model_column: column

            property string model_tableData: (typeof tableData !== "undefined") ? tableData : ({})
            property bool model_selected: (typeof selected !== "undefined") ? selected : ({})
            property bool model_expanded: (typeof expanded !== "undefined") ? expanded : ({})
            property bool model_is_expandable: (typeof is_expandable !== "undefined") ? is_expandable : ({})

            function get_text_left_padding(row, column, isExpandable)
            {
                if (typeof component.get_text_left_padding === "function")
                    return component.get_text_left_padding(row, column, isExpandable)
                else
                    return left_text_padding
            }

            function get_data_color(row, column)
            {
                if (typeof component.get_data_color === "function")
                {
                    var res = component.get_data_color(row, column)
                    if (res!== null)
                        return res
                }

                return (typeof dataColor !== "undefined") ? dataColor : ({})
            }

            Component.onCompleted: {
                delega.update_row_loader_size()
            }
        }

        Connections {
            target: row_loader.item
            function onDeleteRequested (row)
            {
                component.deleteRequested(row)
            }
            function onCopyRequested (tableData)
            {
                component.copyRequested(tableData)
            }
        }

        onWidthChanged: {
            delega.update_row_loader_size()
        }

        onHeightChanged: {
            delega.update_row_loader_size()
        }

        function update_row_loader_size()
        {
            row_loader.width = delega.width
            row_loader.height = childrenRect.height
            if (row_loader.width && row_loader.height && !row_loader.sourceComponent)
            {
                row_loader.sourceComponent = choose_row_source_component(row, column)
            }
        }

        Rectangle {

            anchors.left: parent.left
            anchors.leftMargin: get_line_left_padding(row, column, is_expandable)

            height: 1
            width: parent.width
            color: BSStyle.tableSeparatorColor

            anchors.bottom: parent.bottom
        }
    }

    CustomTableDelegateRow {
        id: cmpnt_table_delegate
    }

    function choose_row_source_component(row, column)
    {
        return cmpnt_table_delegate
    }

    function get_line_left_padding(row, column, isExpandable)
    {
        return 0
    }
}
