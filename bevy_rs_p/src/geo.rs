use bevy_ecs::component::Component;

#[derive(Debug, Clone, Copy, Component)]
pub struct Ecef {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

pub fn distance_ecef(a: Ecef, b: Ecef) -> f64 {
    // ECEF空間でのユークリッド距離を計算します。
    let dx = a.x - b.x;
    let dy = a.y - b.y;
    let dz = a.z - b.z;
    (dx * dx + dy * dy + dz * dz).sqrt()
}

pub fn geodetic_to_ecef(lat_deg: f64, lon_deg: f64, alt_m: f64) -> Ecef {
    // WGS84の測地座標をECEF座標へ変換します。
    let lat = lat_deg.to_radians();
    let lon = lon_deg.to_radians();

    let a = 6378137.0;
    let f = 1.0 / 298.257223563;
    let e2 = f * (2.0 - f);

    let sin_lat = lat.sin();
    let cos_lat = lat.cos();
    let sin_lon = lon.sin();
    let cos_lon = lon.cos();

    let n = a / (1.0 - e2 * sin_lat * sin_lat).sqrt();

    Ecef {
        x: (n + alt_m) * cos_lat * cos_lon,
        y: (n + alt_m) * cos_lat * sin_lon,
        z: (n * (1.0 - e2) + alt_m) * sin_lat,
    }
}

pub fn ecef_to_geodetic(pos: Ecef) -> (f64, f64, f64) {
    // ECEF座標をWGS84の緯度経度高度に戻します（反復近似）。
    let a = 6378137.0;
    let f = 1.0 / 298.257223563;
    let e2 = f * (2.0 - f);

    let p = (pos.x * pos.x + pos.y * pos.y).sqrt();
    let mut lat = (pos.z / p).atan();
    let mut lon = pos.y.atan2(pos.x);
    let mut alt = 0.0;

    for _ in 0..5 {
        let sin_lat = lat.sin();
        let n = a / (1.0 - e2 * sin_lat * sin_lat).sqrt();
        alt = p / lat.cos() - n;
        lat = (pos.z / (p * (1.0 - e2 * n / (n + alt)))).atan();
        lon = pos.y.atan2(pos.x);
    }

    (lat.to_degrees(), lon.to_degrees(), alt)
}
