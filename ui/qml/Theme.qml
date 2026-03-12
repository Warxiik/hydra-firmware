pragma Singleton
import QtQuick

QtObject {
    /* Colors — dark theme with teal accent */
    readonly property color backgroundColor:  "#121212"
    readonly property color surfaceColor:     "#1E1E1E"
    readonly property color cardColor:        "#2A2A2A"
    readonly property color accentColor:      "#00BCD4"  /* Teal */
    readonly property color accentDark:       "#00838F"
    readonly property color textPrimary:      "#FFFFFF"
    readonly property color textSecondary:    "#B0B0B0"
    readonly property color textDisabled:     "#666666"

    /* Nozzle color coding */
    readonly property color nozzle0Color:     "#42A5F5"  /* Blue */
    readonly property color nozzle1Color:     "#FF7043"  /* Orange */

    /* Status colors */
    readonly property color successColor:     "#66BB6A"
    readonly property color warningColor:     "#FFA726"
    readonly property color errorColor:       "#EF5350"

    /* Typography */
    readonly property int fontSizeSmall:    12
    readonly property int fontSizeNormal:   14
    readonly property int fontSizeLarge:    18
    readonly property int fontSizeTitle:    24
    readonly property int fontSizeHuge:     36

    /* Spacing */
    readonly property int spacingSmall:    4
    readonly property int spacingNormal:   8
    readonly property int spacingLarge:    16
    readonly property int spacingXLarge:   24

    /* Touch targets */
    readonly property int touchTarget:     48
    readonly property int buttonHeight:    56

    /* Radius */
    readonly property int cornerRadius:    12
}
