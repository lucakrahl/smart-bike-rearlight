import SwiftUI

/// App-Einstieg. Hält den Composition Root (App Bible 9.5) und reicht ihn per
/// SwiftUI-Environment durch.
@main
struct SmartBikeRearLightApp: App {
    @State private var env = AppEnvironment.live()
    var body: some Scene {
        WindowGroup {
            RootTabView()
                .environment(env)
        }
    }
}
