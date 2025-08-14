import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import de.xplatforms.j2534 1.0



ListView
{
    id: listView

    Layout.fillHeight: true
    Layout.fillWidth: true

    model: ExJ2534Wrapper.getWriteMsgsList()

    focus: true
    currentIndex: -1
    property int lastIndex: -1
    property bool _loaded: false
    spacing: 0

    highlight: Rectangle
    {
        color: "#d4e8f6";
        radius: 0
    }

    flickableDirection: Flickable.AutoFlickDirection

    //highlightFollowsCurrentItem: true
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
        height: 28
        width: listView.width
        border.color: "lightGray"
        border.width: 0
        //radius: 2

        RowLayout
        {
            anchors.fill: parent

            Label
            {
                text: model.Timestamp
                padding: 4
                Layout.fillHeight: true
                //Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                font.family: "Segoe UI" //"AMS New Digital"
                font.pointSize: 11
                color: model.HasError?"red":"grey"
            }

            Label
            {
                padding: 4
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: model.Message
                font.family: "Segoe UI" //"Candara"
                font.pointSize: 11
                color: model.HasError?"red":"#af008c00"

            }

            Rectangle
            {
                Layout.fillHeight: true
                width: 100
                //color: "blue"
            }

        }

        //color: "red"

    }

    ScrollIndicator.vertical: ScrollIndicator { }

}

