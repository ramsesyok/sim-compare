use crate::geo::geodetic_to_ecef;
use crate::geo::Ecef;
use crate::scenario::Scenario;
use crate::scenario::Waypoint;
use crate::sim::{ObjectState, RoutePoint};
use std::collections::HashMap;

pub fn build_objects(scenario: &Scenario) -> Vec<ObjectState> {
    let mut objects = Vec::new();

    // チームとオブジェクトを走査し、AoSの状態配列を組み立てます。
    for team in &scenario.teams {
        for obj in &team.objects {
            // 経路の緯度経度をECEF座標に変換して保持します。
            let route = build_route(&obj.route);
            // 区間ごとの移動時間を計算し、後で位置補間に使います。
            let (segment_end_secs, total_duration_sec) = build_segment_times(&route);
            let position_ecef = route.first().map(|p| p.ecef).unwrap_or(Ecef {
                x: 0.0,
                y: 0.0,
                z: 0.0,
            });

            objects.push(ObjectState {
                id: obj.id.clone(),
                team_id: team.id.clone(),
                role: obj.role,
                start_sec: obj.start_sec,
                route,
                segment_end_secs,
                total_duration_sec,
                position_ecef,
                detect_state: HashMap::new(),
                has_detonated: false,
            });
        }
    }

    objects
}

fn build_route(route: &[Waypoint]) -> Vec<RoutePoint> {
    route
        .iter()
        .map(|wp| {
            // WGS84の緯度経度高度をECEF座標へ変換します。
            let ecef = geodetic_to_ecef(wp.lat_deg, wp.lon_deg, wp.alt_m);
            RoutePoint {
                lat_deg: wp.lat_deg,
                lon_deg: wp.lon_deg,
                alt_m: wp.alt_m,
                speeds_kph: wp.speeds_kph,
                ecef,
            }
        })
        .collect()
}

fn build_segment_times(route: &[RoutePoint]) -> (Vec<f64>, f64) {
    if route.len() < 2 {
        return (Vec::new(), 0.0);
    }

    let mut segment_end_secs = Vec::with_capacity(route.len() - 1);
    let mut acc = 0.0;

    for i in 0..route.len() - 1 {
        // 区間距離と速度から移動に必要な秒数を算出します。
        let a = route[i].ecef;
        let b = route[i + 1].ecef;
        let distance_m = crate::geo::distance_ecef(a, b);
        let speed_mps = (route[i].speeds_kph * 1000.0) / 3600.0;
        let duration = if speed_mps <= 0.0 {
            f64::INFINITY
        } else {
            distance_m / speed_mps
        };
        acc += duration;
        segment_end_secs.push(acc);
    }

    (segment_end_secs, acc)
}
