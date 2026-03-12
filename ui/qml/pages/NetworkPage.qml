import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    background: Rectangle { color: Theme.backgroundColor }

    Label {
        anchors.centerIn: parent
        text: "NetworkPage"
        font.pixelSize: Theme.fontSizeTitle
        color: Theme.textPrimary
    }
}
