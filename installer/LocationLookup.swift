import CoreLocation
import Foundation

final class LocationLookup: NSObject, CLLocationManagerDelegate {
    private let manager = CLLocationManager()
    private var timer: Timer?
    private var completion: ((String?, Double?, String) -> Void)?
    override init() { super.init(); manager.delegate = self; manager.desiredAccuracy = kCLLocationAccuracyHundredMeters }
    func start(_ callback: @escaping (String?, Double?, String) -> Void) {
        cancel(); completion = callback
        guard CLLocationManager.locationServicesEnabled() else { finish(nil, nil, "Location Services is off. Find your grid on the map or enter it yourself."); return }
        timer = Timer.scheduledTimer(withTimeInterval: 20, repeats: false) { [weak self] _ in self?.finish(nil, nil, "Location was unavailable. Find your grid on the map or enter it yourself.") }
        switch manager.authorizationStatus {
        case .denied, .restricted: finish(nil, nil, "Location access is unavailable. Find your grid on the map or enter it yourself.")
        case .notDetermined: manager.requestWhenInUseAuthorization()
        default: manager.startUpdatingLocation()
        }
    }
    func cancel() { timer?.invalidate(); timer = nil; completion = nil; manager.stopUpdatingLocation() }
    private func finish(_ grid: String?, _ accuracy: Double?, _ message: String) {
        let callback = completion; cancel(); callback?(grid, accuracy, message)
    }
    func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
        guard completion != nil else { return }
        switch manager.authorizationStatus {
        case .denied, .restricted: finish(nil, nil, "Location access was declined. You can still enter your grid or find it on the map.")
        case .notDetermined: break
        default: manager.startUpdatingLocation()
        }
    }
    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let location = locations.last else { return }
        guard let grid = GridLocator.suggestion(latitude: location.coordinate.latitude, longitude: location.coordinate.longitude, accuracy: location.horizontalAccuracy, age: -location.timestamp.timeIntervalSinceNow) else { return }
        finish(grid, location.horizontalAccuracy, "Confirm that this is where your receiver is located before installing.")
    }
    func locationManager(_ manager: CLLocationManager, didFailWithError error: Error) {
        if (error as? CLError)?.code == .locationUnknown { return }
        finish(nil, nil, "Location was unavailable. Find your grid on the map or enter it yourself.")
    }
}
