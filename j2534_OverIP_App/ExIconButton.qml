import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle
{
    id: _comp_rect_id

    property int icon_h: 24
    property int icon_w: 24
    property bool active: false
    property alias text: _comp_labl_id.text
    property alias icon_source: _comp_btn_id.icon.source

    property alias show_label: _comp_labl_id.visible

    signal itemClicked()

    border.width: 0
    radius: 4
    color: active? "#d4e8f6" : "transparent"

    RowLayout
    {
        anchors.fill: parent

        Button
        {
            id: _comp_btn_id
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignLeft

            display: AbstractButton.IconOnly

            flat: true
            //icon.source: "qrc:/assets/garage.svg"
            icon.height: _comp_rect_id.icon_h
            icon.width: _comp_rect_id.icon_w
            background: Rectangle
            {
                implicitHeight: _comp_rect_id.icon_h
                implicitWidth: _comp_rect_id.icon_w
                color: "transparent"
            }
        }

        Label
        {
            id: _comp_labl_id
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: Qt.AlignVCenter
            font.family: "Segoe UI"
        }
    }

    MouseArea
    {
        anchors.fill: parent
        onClicked: itemClicked()
    }
}
