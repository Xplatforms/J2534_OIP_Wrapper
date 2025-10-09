import QtQuick
import QtQuick.Controls.Universal
import QtQuick.Layouts

ApplicationWindow
{
    id: window
    width: 1200
    height: 720
    visible: true
    title: qsTr("J2534 OverIP App - Xplatforms")

    Action
    {
        id: navigateBackAction
        icon.name: stackViewMain.depth > 1 ? "back" : "drawer"
        onTriggered: {
            if (stackViewMain.depth > 1) {
                stackViewMain.pop()
                listView.currentIndex = -1
            } else {
                //drawer.open()
            }
        }
    }

    Shortcut
    {
        sequence: "Menu"
        onActivated: optionsMenuAction.trigger()
    }

    Action
    {
        id: optionsMenuAction
        icon.name: "menu"
        onTriggered: optionsMenu.open()
    }

    Component.onCompleted: function(){
        switch(ExSelectedInterface)
        {
        case 0:
            console.log("SimpleLogger")
            stackViewMain.push("TestPage.qml")
            break
        case 2:
            console.log("J2534DllWrapper")
            stackViewMain.push("J2534Wrapper.qml")
            break
        case 3:
            console.log("J2534Emulator")
            stackViewMain.push("J2534Emulator.qml")
            break
        case 4:
            console.log("J2534Cannelloni")
            stackViewMain.push("J2534Cannelloni.qml")
            break
        }
    }

/*
    Drawer
    {
        id: drawer

        width: (window.width * 0.33)<319?319:(window.width * 0.33)
        height: window.height - 2
        leftMargin: 2
        leftInset: 1
        y:1

        interactive: true //stackViewMain.depth === 1

        ColumnLayout
        {
            spacing: 10
            anchors.fill: parent

            Rectangle
            {
                Layout.fillHeight: true
                Layout.fillWidth: true
                z:1

                ColumnLayout
                {
                    anchors.fill: parent
                    anchors.topMargin: 30
                    RowLayout
                    {
                        Layout.leftMargin: 20
                        Image
                        {
                            sourceSize.width: 20
                            sourceSize.height: 20
                            //source: "qrc:/images/noun-menu-928567-0059B3.svg"
                        }
                        Label
                        {
                            Layout.leftMargin: 15
                            text: "J2534 OverIP App"
                        }
                    }

                    Rectangle
                    {
                        height: 30
                    }

                    ListView
                    {
                        id: listView

                        focus: true
                        currentIndex: -1
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        highlight: Rectangle { color: "#cfdbeb"; radius: 2 }
                        highlightFollowsCurrentItem: true
                        clip: true

                        delegate: ItemDelegate {
                            width: listView.width
                            text: model.title
                            highlighted: ListView.isCurrentItem
                            onClicked: {
                                listView.currentIndex = index
                                stackViewMain.push(model.source)
                                drawer.close()
                            }
                        }

                        model: ListModel {
                            ListElement { title: "Main"; source: "MainPage.qml" }
                            ListElement { title: "Test"; source: "TestPage.qml" }
                        }

                        ScrollIndicator.vertical: ScrollIndicator { }
                    }
                }
            }
        }
    }
*/
    Rectangle
    {
        id: mainrect
        anchors.fill: parent

        ColumnLayout
        {
            spacing: 0
            anchors.fill: parent
            anchors.topMargin: 1
            anchors.rightMargin: 1
            anchors.leftMargin: 1
            anchors.bottomMargin: 1

            RowLayout
            {
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.margins: 0
                spacing: 0
            }

            StackView
            {
                id: stackViewMain
                Layout.leftMargin: 0
                Layout.rightMargin: 0
                Layout.fillHeight: true
                Layout.fillWidth: true

                onCurrentItemChanged:
                {
                    console.log("currentItem " + currentItem.name_id)
                    if(currentItem.name_id === 2)
                    {

                    }
                    else if(currentItem.name_id === 1)
                    {

                    }
                    else if(currentItem.name_id === 700)
                    {

                    }
                }

                initialItem: TestPage{}

                //initialItem: MainPage{}

                pushEnter: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 0
                        to:1
                        duration: 200
                    }
                }
                pushExit: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 1
                        to:0
                        duration: 200
                    }
                }
                popEnter: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 0
                        to:1
                        duration: 200
                    }
                }
                popExit: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 1
                        to:0
                        duration: 200
                    }
                }
            }

            RowLayout
            {
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.margins: 0
                spacing: 0
            }
        }

    }
}
