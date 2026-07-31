// swift-tools-version: 6.0
// SmartBikeCore — reine, UI-freie Logik der App (App Bible Kap. 9.7).
// Host-testbar auf dem Mac (AR-NFR-TST-01), ohne SwiftUI / Core Bluetooth / SwiftData.
import PackageDescription

let package = Package(
    name: "SmartBikeCore",
    platforms: [.iOS(.v18), .macOS(.v14)],
    products: [
        .library(name: "SmartBikeCore", targets: ["SmartBikeCore"])
    ],
    targets: [
        .target(name: "SmartBikeCore"),
        .testTarget(name: "SmartBikeCoreTests", dependencies: ["SmartBikeCore"])
    ]
)
