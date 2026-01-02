use crate::geo::{distance_ecef, geodetic_to_ecef, Ecef};
use crate::scenario::{Scenario, Waypoint};
use crate::sim::{RoutePoint, SoaState};
use std::collections::HashMap;

pub fn build_state(scenario: &Scenario) -> SoaState {
    let mut ids = Vec::new();
    let mut team_ids = Vec::new();
    let mut roles = Vec::new();
    let mut start_secs = Vec::new();
    let mut routes = Vec::new();
    let mut segment_end_secs = Vec::new();
    let mut total_duration_secs = Vec::new();
    let mut positions = Vec::new();
    let mut detect_state = Vec::new();
    let mut has_detonated = Vec::new();

    // SoAは「同じ種類のデータを配列でまとめる」設計です。
    // ここでは、ID配列・役割配列・位置配列などを個別に保持します。
    for team in &scenario.teams {
        for obj in &team.objects {
            // 経路の緯度経度をECEF座標に変換して保持します。
            let route = build_route(&obj.route);
            // 区間ごとの移動時間を計算し、後で位置補間に使います。
            let (segment_ends, total_duration_sec) = build_segment_times(&route);
            let position = route.first().map(|p| p.ecef).unwrap_or(Ecef {
                x: 0.0,
                y: 0.0,
                z: 0.0,
            });

            ids.push(obj.id.clone());
            team_ids.push(team.id.clone());
            roles.push(obj.role);
            start_secs.push(obj.start_sec);
            routes.push(route);
            segment_end_secs.push(segment_ends);
            total_duration_secs.push(total_duration_sec);
            positions.push(position);
            detect_state.push(HashMap::new());
            has_detonated.push(false);
        }
    }

    SoaState {
        ids,
        team_ids,
        roles,
        start_secs,
        routes,
        segment_end_secs,
        total_duration_secs,
        positions,
        detect_state,
        has_detonated,
    }
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
        let distance_m = distance_ecef(a, b);
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
