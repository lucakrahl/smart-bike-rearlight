import SwiftUI

/// Liquid Glass für die schwebende Chrome-/Steuerungsebene (iOS 26).
/// GRUNDSATZ (App Bible 8 / AR-UX-01): NUR für Steuerelemente und schwebende
/// Pillen einsetzen — nie für Inhalts-/Datenflächen (Kacheln, Charts, Listen).
/// Fällt unter iOS 26 auf `.ultraThinMaterial` zurück, damit die App baut/läuft.
extension View {
    /// Legt einen Liquid-Glass-Hintergrund in `shape` hinter das (bereits fertig
    /// gelayoutete) Steuerelement. `interactive` nur für tatsächlich tippbare Elemente.
    /// WICHTIG: erst nach den Layout-Modifiern (padding/frame) anwenden.
    @ViewBuilder
    func floatingGlass(interactive: Bool = false, in shape: some Shape = Capsule()) -> some View {
        if #available(iOS 26.0, *) {
            self.glassEffect(interactive ? .regular.interactive() : .regular, in: shape)
        } else {
            self.background(.ultraThinMaterial, in: shape)
        }
    }
}
