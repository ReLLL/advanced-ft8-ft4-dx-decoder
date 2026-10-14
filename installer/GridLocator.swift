import Foundation

struct GridLocator {
    static func coordinate(latitude: Double, longitude: Double, sixCharacters: Bool = true) -> String? {
        guard latitude.isFinite, longitude.isFinite, (-90...90).contains(latitude), (-180...180).contains(longitude) else { return nil }
        let x = min(longitude + 180, 360.0.nextDown), y = min(latitude + 90, 180.0.nextDown)
        let letters = Array("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
        var grid = "\(letters[Int(x / 20)])\(letters[Int(y / 10)])\(Int(x / 2) % 10)\(Int(y) % 10)"
        if sixCharacters { grid += "\(letters[Int(x.truncatingRemainder(dividingBy: 2) * 12)])\(letters[Int(y.truncatingRemainder(dividingBy: 1) * 24)])" }
        return grid
    }
    static func suggestion(latitude: Double, longitude: Double, accuracy: Double, age: Double) -> String? {
        guard accuracy.isFinite, accuracy >= 0, accuracy <= 20_000, age.isFinite, age >= -30, age <= 300 else { return nil }
        return coordinate(latitude: latitude, longitude: longitude, sixCharacters: accuracy <= 5_000)
    }
}
