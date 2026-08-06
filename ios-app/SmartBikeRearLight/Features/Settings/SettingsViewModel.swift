import Foundation
import Observation

@MainActor @Observable
final class SettingsViewModel {
    /// App-Version aus dem Bundle (Info-Sektion).
    static var appVersion: String {
        (Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String) ?? "?"
    }
}
