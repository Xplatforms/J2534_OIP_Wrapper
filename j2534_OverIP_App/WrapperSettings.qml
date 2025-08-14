import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import de.xplatforms.j2534 1.0

Item {

    Component.onCompleted: {
        ExJ2534Wrapper.exportToJSON()
    }

    Label{
        text: "SETTINGS"
    }

}
