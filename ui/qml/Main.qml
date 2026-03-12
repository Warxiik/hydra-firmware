import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "Hydra"
    color: Theme.backgroundColor

    Material.theme: Material.Dark
    Material.accent: Theme.accentColor

    /* Top status bar */
    header: StatusBar {
        id: statusBar
    }

    /* Page stack */
    StackView {
        id: pageStack
        anchors.fill: parent
        anchors.topMargin: 0
        initialItem: homePage
    }

    /* Bottom navigation */
    footer: ToolBar {
        Material.background: Theme.surfaceColor
        height: 56

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: [
                    { name: "Home", icon: "qrc:/icons/home.svg", page: homePage },
                    { name: "Prepare", icon: "qrc:/icons/tune.svg", page: preparePage },
                    { name: "Print", icon: "qrc:/icons/print.svg", page: printPage },
                    { name: "Files", icon: "qrc:/icons/folder.svg", page: filesPage },
                    { name: "Settings", icon: "qrc:/icons/settings.svg", page: settingsPage }
                ]

                delegate: ToolButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: modelData.name
                    font.pixelSize: 12
                    display: AbstractButton.TextUnderIcon
                    // icon.source: modelData.icon
                    onClicked: pageStack.replace(null, modelData.page)
                }
            }
        }
    }

    /* Page components */
    Component { id: homePage; HomePage {} }
    Component { id: preparePage; PreparePage {} }
    Component { id: printPage; PrintPage {} }
    Component { id: filesPage; FilesPage {} }
    Component { id: settingsPage; SettingsPage {} }
    Component { id: tunePage; TunePage {} }
    Component { id: temperaturePage; TemperaturePage {} }
    Component { id: movePage; MovePage {} }
    Component { id: networkPage; NetworkPage {} }
}
