use crate::geo::{distance_ecef, geodetic_to_ecef, Ecef};
use crate::scenario::{Scenario, Waypoint};
use crate::sim::{
    DetectState, HasDetonated, Id, RoutePoint, RoutePoints, SegmentEndSecs, StartSec, TeamId,
    TotalDurationSec,
};
use hecs::{Entity, World};

pub fn build_world(scenario: &Scenario) -> (World, Vec<Entity>) {
    let mut world = World::new();
    let mut entities = Vec::new();

    // ECSでは「要素ごとにコンポーネントを分ける」設計です。
    // ここでは各オブジェクトをエンティティとして生成します。
    for team in &scenario.teams {
        for obj in &team.objects {
            let route = build_route(&obj.route);
            let (segment_end_secs, total_duration_sec) = build_segment_times(&route);
            let position = route.first().map(|p| p.ecef).unwrap_or(Ecef {
                x: 0.0,
                y: 0.0,
                z: 0.0,
            });

            let entity = world.spawn((
                Id(obj.id.clone()),
                TeamId(team.id.clone()),
                obj.role,
                StartSec(obj.start_sec),
                RoutePoints(route),
                SegmentEndSecs(segment_end_secs),
                TotalDurationSec(total_duration_sec),
                position,
                DetectState::default(),
                HasDetonated(false),
            ));
            entities.push(entity);
        }
    }

    (world, entities)
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
