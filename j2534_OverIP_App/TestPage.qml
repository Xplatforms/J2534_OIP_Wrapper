import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import de.xplatforms.j2534 1.0

Pane
{
    id: _test_page_id
    property int name_id: 333

    padding: 0

    focusPolicy: Qt.ClickFocus

    ColumnLayout
    {
        anchors.fill: parent
        spacing: 0
        Layout.margins: 0

        RowLayout
        {
            Layout.fillWidth: true
            Layout.leftMargin: 0
            Layout.topMargin: 0

            ListView {
                Layout.fillHeight: true
                Layout.fillWidth: true

                model: Qt.fontFamilies()

                delegate: Item {
                    height: 40;
                    width: ListView.view.width
                    Text {
                        font.family: modelData
                        anchors.centerIn: parent
                        text: modelData;
                    }
                }
            }

        }
    }

}
