import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import de.xplatforms.j2534 1.0

Pane
{
    id: _j2534_cannelloni
    property int name_id: 2

    padding: 0

    focusPolicy: Qt.ClickFocus

    Connections
    {
        target: leftBox
        function onCurr_filterChanged()
        {
            console.log("onCurr_filterChanged " + leftBox.curr_filter)
            switch(leftBox.curr_filter)
            {
            case 1:
                content_stack.replace(null, "CannelloniMainView.qml", {});
                break;
            case 2:
                content_stack.replace(null, "CannelloniRXView.qml", {});
                break;
            case 3:
                content_stack.replace(null, "CannelloniTXView.qml", {});
                break;
            case 4:
                content_stack.replace(null, "CannelloniRXTXQueuesView.qml", {});
                break;
            case 5:
                content_stack.replace(null, "CannelloniSettings.qml", {});
                break;
            }
        }
    }

    GridLayout
    {
        anchors.fill: parent
        columns: 3//(window.width > 768)? 3 : 1;
        rows: 1

        states: [
            State {
                when: window.width <= 600
                PropertyChanges { target: leftBox; Layout.columnSpan: 1 }
                PropertyChanges { target: contentBox; Layout.columnSpan: 1 }
                PropertyChanges { target: rightBox; Layout.columnSpan: 1 }
            },
            State {
                when: window.width > 600 && window.width <= 768
                PropertyChanges { target: leftBox; Layout.columnSpan: 1 }
                PropertyChanges { target: contentBox; Layout.columnSpan: 1 }
                PropertyChanges { target: rightBox; Layout.columnSpan: 1 }
            },
            State {
                when: window.width > 768
                PropertyChanges { target: leftBox; Layout.columnSpan: 1 }
                PropertyChanges { target: contentBox; Layout.columnSpan: 1 }
                PropertyChanges { target: rightBox; Layout.columnSpan: 1 }
            }
        ]

        RowLayout
        {
            id: leftBox

            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: collapsed?50:180
            Layout.maximumWidth: collapsed?50:180
            Layout.minimumWidth: 50

            property bool collapsed: false
            property int curr_filter: 1

            ColumnLayout
            {
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.leftMargin: 4

                RowLayout
                {
                    Layout.fillWidth: true

                    Rectangle
                    {
                        visible: !leftBox.collapsed
                        Layout.fillWidth: true
                        color: "transparent"
                    }

                    Button{
                        Layout.leftMargin: 4
                        Layout.alignment: Qt.AlignRight
                        flat: true
                        width: 16
                        icon.source: leftBox.collapsed?"qrc:/assets/arrow2_right.png":"qrc:/assets/arrow2_left.png"
                        icon.width: 16
                        icon.height: 16

                        onClicked: {
                            leftBox.collapsed = !leftBox.collapsed
                        }
                    }
                }

                ExIconButton
                {
                    Layout.fillWidth: true
                    height: 30

                    show_label: !leftBox.collapsed
                    active: leftBox.curr_filter === 1
                    icon_source: "qrc:/assets/garage.svg"
                    text: "Dashboard"

                    onItemClicked: {
                        leftBox.curr_filter = 1
                    }
                }

                ExIconButton
                {
                    Layout.fillWidth: true
                    height: 30

                    show_label: !leftBox.collapsed
                    active: leftBox.curr_filter === 2
                    icon_source: "qrc:/assets/arrow_left.png"
                    text: "(RX) ReadMsg's"

                    onItemClicked: {
                        leftBox.curr_filter = 2
                    }
                }

                ExIconButton
                {
                    Layout.fillWidth: true
                    height: 30

                    show_label: !leftBox.collapsed
                    active: leftBox.curr_filter === 3
                    icon_source: "qrc:/assets/arrow_right.png"
                    text: "(TX) WriteMsg's"

                    onItemClicked: {
                        leftBox.curr_filter = 3
                    }
                }

                ExIconButton
                {
                    Layout.fillWidth: true
                    height: 30

                    show_label: !leftBox.collapsed
                    active: leftBox.curr_filter === 4
                    icon_source: "qrc:/assets/arrows.svg"
                    text: "RX / TX Queue"

                    onItemClicked: {
                        leftBox.curr_filter = 4
                    }
                }

                Rectangle
                {
                    Layout.fillHeight: true
                    color: "transparent"
                }

                ExIconButton
                {
                    Layout.fillWidth: true
                    height: 30

                    show_label: !leftBox.collapsed
                    active: leftBox.curr_filter === 5
                    icon_source: "qrc:/assets/settings_icon.png"
                    text: "Settings"

                    onItemClicked: {
                        leftBox.curr_filter = 5
                    }
                }

                Rectangle
                {
                    height: 10
                    color: "transparent"
                }
            }

            Rectangle
            {
                width: 1
                Layout.fillHeight: true
                color: "lightgrey"
                opacity: 0.3
            }
        }


        ColumnLayout
        {
            id: contentBox

            Layout.fillHeight: true
            Layout.fillWidth: true

            RowLayout
            {
                Layout.fillWidth: true
                Label
                {
                    Layout.topMargin: 2
                    Layout.bottomMargin: 2
                    Layout.fillWidth: true
                    horizontalAlignment: Qt.AlignHCenter
                    font.family: "Segoe UI"
                    text: "J2534 Cannelloni Client"
                }
            }

            StackView
            {
                id: content_stack
                Layout.fillWidth: true
                Layout.fillHeight: true

                initialItem: CannelloniMainView{}
            }
        }

        RowLayout
        {
            id: rightBox

            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 320
            Layout.maximumWidth: 320
            Layout.minimumWidth: 48

            Rectangle
            {
                width: 1
                Layout.fillHeight: true
                color: "lightgrey"
                opacity: 0.3
            }

            ColumnLayout
            {
                Layout.fillHeight: true
                Layout.fillWidth: true

                Rectangle
                {
                    Layout.fillWidth: true
                    height: 10
                    color: "transparent"
                }

                Label
                {
                    Layout.fillWidth: true
                    horizontalAlignment: Qt.AlignHCenter
                    font.weight: Font.DemiBold
                    font.family: "Segoe UI"
                    text: "Wrapped functions:"
                }

                RowLayout
                {
                    id: _row_funcs_btn
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    property bool load_state: true

                    Button
                    {
                        id: _loaded_btn
                        flat: true
                        text: "loaded"

                        contentItem: Text
                        {
                                 text: _loaded_btn.text
                                 font: _loaded_btn.font
                                 color: _loaded_btn.hovered ? "white" : "black"
                                 horizontalAlignment: Text.AlignHCenter
                                 verticalAlignment: Text.AlignVCenter
                                 elide: Text.ElideRight
                        }

                        background: Rectangle
                        {
                                 implicitWidth: 120
                                 implicitHeight: 20
                                 opacity: enabled ? 1 : 0.3
                                 color: _loaded_btn.hovered ? "#17a81a" : "white"
                                 border.color: _row_funcs_btn.load_state ? "#17a81a" : "white"
                                 border.width: 1
                                 radius: 4
                        }

                        onClicked:
                        {
                            _row_funcs_btn.load_state = true
                        }
                    }

                    Button
                    {
                        id: _unloaded_btn
                        flat: true
                        text: "unloaded"

                        contentItem: Text
                        {
                                 text: _unloaded_btn.text
                                 font: _unloaded_btn.font
                                 color: _unloaded_btn.hovered ? "white" : "black"
                                 horizontalAlignment: Text.AlignHCenter
                                 verticalAlignment: Text.AlignVCenter
                                 elide: Text.ElideRight
                        }

                        background: Rectangle
                        {
                                 implicitWidth: 120
                                 implicitHeight: 20
                                 opacity: enabled ? 1 : 0.3
                                 color: _unloaded_btn.hovered ? "#17a81a" : "white"
                                 border.color: !_row_funcs_btn.load_state ? "#17a81a" : "white"
                                 border.width: 1
                                 radius: 4
                        }

                        onClicked:
                        {
                            _row_funcs_btn.load_state = false
                        }
                    }
                }

                //
/*
                ListView
                {
                    id: _func_list_view_id
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.leftMargin: 4
                    Layout.topMargin: 10


                    clip: true

                    model: _row_funcs_btn.load_state?ExJ2534Wrapper.loaded_dll_funcs:ExJ2534Wrapper.unloaded_dll_funcs

                    delegate: Rectangle
                    {
                        width: _func_list_view_id.width
                        height: 30

                        Label
                        {
                            anchors.fill: parent
                            font.family: "Segoe UI"
                            font.pointSize: 10
                            text: display
                        }
                    }
                }

                */
            }
        }


    }
}
