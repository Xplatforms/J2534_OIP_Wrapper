import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

import de.xplatforms.j2534 1.0

Rectangle {
    //anchors.fill: parent
    // The background color will show through the cell
    // spacing, and therefore become the grid line color.
    color: Qt.styleHints.appearance === Qt.Light ? palette.mid : palette.midlight



    HorizontalHeaderView {
        id: horizontalHeader
        anchors.left: tableView.left
        anchors.top: parent.top
        syncView: tableView
        clip: true
        textRole: MSGSTableViewModel.header_name
    }

    VerticalHeaderView {
        id: verticalHeader
        anchors.top: tableView.top
        anchors.left: parent.left
        syncView: tableView
        clip: true
    }

    TableView {
        id: tableView
        anchors.left: verticalHeader.right
        anchors.top: horizontalHeader.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        //anchors.leftMargin: verticalHeader.width+2
        //anchors.topMargin: horizontalHeader.height+2

        property var columnWidths: [150, 150, 150, 150]
        columnWidthProvider: function (column) { return columnWidths[column] }

        Timer {
            running: true
            interval: 200
            onTriggered: {
                //tableView.columnWidths[2] = 150
                tableView.forceLayout();
            }
        }


        model: MSGSTableViewModel{}

        delegate: Rectangle {

            implicitWidth: 150
            implicitHeight: 40
            border.width: 0

            Text {
                Layout.fillWidth: true
                text: display
                anchors.centerIn: parent
            }
        }
    }
}
