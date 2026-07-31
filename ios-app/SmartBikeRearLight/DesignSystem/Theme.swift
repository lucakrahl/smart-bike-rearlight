import SwiftUI

/// Designsystem (App Bible Kap. 8 / UX-A). Farben adaptiv (Hell/Dunkel),
/// „Electric Cyan"-Akzent liegt als AccentColor in Assets.xcassets.
enum Theme {
    enum Spacing { static let unit: CGFloat = 8 }          // 8-pt-Raster
    static let tileCornerRadius: CGFloat = 16
    static let minTapTarget: CGFloat = 44

    enum Semantic {
        static let warning = Color.red      // Warnung/Bremse/Fehler
        static let ok = Color.green         // Fix ok
        static let searching = Color.orange // Suche/kein Fix
    }
    enum Chart {
        static let speed = Color.accentColor          // Cyan
        static let altitude = Color(red: 0.39, green: 0.45, blue: 0.55) // Slate
    }
    /// Cockpit-Ziffern: SF Pro Rounded, tabellarische Ziffern (UX-A).
    static func numeric(_ size: CGFloat) -> Font {
        .system(size: size, weight: .bold, design: .rounded).monospacedDigit()
    }
}
