import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import de.xplatforms.j2534 1.0

Pane
{
    id: _j2534_emulator
    property int name_id: 2

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

            ListView
            {
                id: listView

                focus: true
                currentIndex: -1
                property int lastIndex: -1
                property bool _loaded: false
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 2

                highlight: Rectangle
                {
                    color: "#d4e8f6";
                    radius: 0
                }

                flickableDirection: Flickable.AutoFlickDirection

                highlightFollowsCurrentItem: true
                highlightMoveDuration: 100
                clip: true

                Component.onCompleted:
                {
                    incrementCurrentIndex()
                    console.log("Component.onCompleted " + listView.currentIndex + " count " + listView.count)

                    forceLayout()
                    listView._loaded = true;
                }

                onCurrentItemChanged:
                {

                    if(!listView._loaded || listView.count === 0)return;
                    if(listView.lastIndex > -1 && listView.itemAtIndex(listView.lastIndex) !== null)
                    {
                        listView.itemAtIndex(listView.lastIndex).cur_selected = false
                    }

                    listView.lastIndex = listView.currentIndex
                    listView.itemAtIndex(listView.currentIndex).cur_selected = true

                    forceActiveFocus()
                }

                delegate: Rectangle
                {
                    height: 64
                    width: listView.width

                    RowLayout
                    {
                        anchors.fill: parent
                        Label
                        {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            text: model.Message
                        }
                    }

                    color: "red"

                }

                model: ExJ2534Emulator

                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }
    }

}
