import QtQuick 2.15
import QtQuick.Controls 2.15

import "../BsStyles"

HorizontalHeaderView {
   id: root
   property int text_size

   delegate: Rectangle {

      implicitHeight: 34
      implicitWidth: 100
      color: BSStyle.tableCellBackgroundColor

      Text {
         text: display
         height: parent.height
         verticalAlignment: Text.AlignVCenter
         clip: true
         color: BSStyle.titleTextColor
         font.family: "Roboto"
         font.weight: Font.Normal
         font.pixelSize: root.text_size
         leftPadding: 10
      }

      Rectangle {
         height: 1
         width: parent.width
         color: BSStyle.tableSeparatorColor

         anchors.bottom: parent.bottom
      }
   }
}
