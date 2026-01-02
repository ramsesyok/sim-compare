use crate::geo::{distance_ecef, ecef_to_geodetic, Ecef};
use crate::log::{DetectionEvent, DetonationEvent};
use crate::scenario::Role;
use crate::spatial::{cell_key, CellKey};
use bevy_ecs::prelude::{Component, Entity, Query, Res, ResMut, Resource};
use std::collections::{HashMap, HashSet};

#[derive(Debug, Clone, Component)]
pub struct Id(pub String);

#[derive(Debug, Clone, Component)]
pub struct TeamId(pub String);

#[derive(Debug, Clone, Component)]
pub struct StartSec(pub i64);

#[derive(Debug, Clone, Component)]
pub struct SegmentEndSecs(pub Vec<f64>);

#[derive(Debug, Clone, Component)]
pub struct TotalDurationSec(pub f64);

#[derive(Debug, Clone, Default, Component)]
pub struct DetectState(pub HashMap<String, DetectionInfo>);

#[derive(Debug, Clone, Component)]
pub struct HasDetonated(pub bool);

#[derive(Debug, Clone)]
// TODO: 移動処理で使うようになったら警告抑制を解除します。
#[allow(dead_code)]
pub struct RoutePoint {
    // 緯度経度高度はデバッグや将来の補助計算で利用する予定です。
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub speeds_kph: f64,
    pub ecef: Ecef,
}

#[derive(Debug, Clone, Component)]
pub struct RoutePoints(pub Vec<RoutePoint>);

#[derive(Debug, Clone)]
pub struct DetectionInfo {
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub distance_m: i64,
}

#[derive(Debug, Clone, Copy, Resource)]
pub struct SimTime {
    pub time_sec: i64,
}

#[derive(Debug, Clone, Copy, Resource)]
pub struct DetectRange(pub f64);

#[derive(Debug, Clone, Copy, Resource)]
pub struct BomRange(pub i64);

#[derive(Debug, Default, Resource)]
pub struct SnapshotCache {
    pub snapshots: Vec<EntitySnapshot>,
    pub spatial_hash: HashMap<CellKey, Vec<usize>>,
}

#[derive(Debug, Default, Resource)]
pub struct EventBuffer {
    pub detection_events: Vec<DetectionEvent>,
    pub detonation_events: Vec<DetonationEvent>,
}

#[derive(Debug, Clone)]
pub struct EntitySnapshot {
    pub entity: Entity,
    pub id: String,
    pub team_id: String,
    pub role: Role,
    pub position: Ecef,
}

pub fn position_update_system(
    time: Res<SimTime>,
    mut query: Query<(
        &Role,
        &StartSec,
        &RoutePoints,
        &SegmentEndSecs,
        &TotalDurationSec,
        &mut Ecef,
    )>,
) {
    // hecs同様、BevyでもSystem関数としてクエリ単位の処理を分けます。
    for (role, start_sec, route, segment_end, total_duration, mut position) in query.iter_mut() {
        *position = position_at_time(
            *role,
            start_sec.0,
            &route.0,
            &segment_end.0,
            total_duration.0,
            time.time_sec as f64,
        );
    }
}

fn position_at_time(
    role: Role,
    start_sec: i64,
    route: &[RoutePoint],
    segment_end_secs: &[f64],
    total_duration_sec: f64,
    time_sec: f64,
) -> Ecef {
    // 司令官は移動しないので、最初の経路点に固定します。
    if role == Role::Commander {
        return route.first().map(|p| p.ecef).unwrap_or(Ecef {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        });
    }

    if route.is_empty() {
        return Ecef {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        };
    }

    // 移動開始前は最初の経路点に待機します。
    if time_sec < start_sec as f64 {
        return route[0].ecef;
    }

    // 経路が1点だけのときは、その位置に固定します。
    if segment_end_secs.is_empty() {
        return route[route.len() - 1].ecef;
    }

    // 全区間を移動し終えた場合は最終点に固定します。
    let elapsed = time_sec - start_sec as f64;
    if elapsed >= total_duration_sec {
        return route[route.len() - 1].ecef;
    }

    // 経過時間から現在の区間を探し、線形補間で位置を計算します。
    let mut segment_index = 0;
    while segment_index < segment_end_secs.len() && elapsed > segment_end_secs[segment_index] {
        segment_index += 1;
    }

    if segment_index >= segment_end_secs.len() {
        return route[route.len() - 1].ecef;
    }

    let segment_end = segment_end_secs[segment_index];
    let segment_start = if segment_index == 0 {
        0.0
    } else {
        segment_end_secs[segment_index - 1]
    };
    let segment_duration = segment_end - segment_start;
    if segment_duration <= 0.0 {
        return route[segment_index + 1].ecef;
    }

    let t = (elapsed - segment_start) / segment_duration;
    let a = route[segment_index].ecef;
    let b = route[segment_index + 1].ecef;

    Ecef {
        x: a.x + (b.x - a.x) * t,
        y: a.y + (b.y - a.y) * t,
        z: a.z + (b.z - a.z) * t,
    }
}

pub fn snapshot_system(
    detect_range: Res<DetectRange>,
    mut cache: ResMut<SnapshotCache>,
    query: Query<(Entity, &Id, &TeamId, &Role, &Ecef)>,
) {
    cache.snapshots.clear();
    let mut positions: Vec<Ecef> = Vec::with_capacity(query.iter().count());

    for (entity, id, team_id, role, position) in query.iter() {
        cache.snapshots.push(EntitySnapshot {
            entity,
            id: id.0.clone(),
            team_id: team_id.0.clone(),
            role: *role,
            position: *position,
        });
        positions.push(*position);
    }

    // 探知処理のため、位置の配列から空間ハッシュを構築します。
    cache.spatial_hash = crate::spatial::build_spatial_hash(&positions, detect_range.0);
}

pub fn detection_system(
    time: Res<SimTime>,
    detect_range: Res<DetectRange>,
    cache: Res<SnapshotCache>,
    mut detect_query: Query<&mut DetectState>,
    mut events: ResMut<EventBuffer>,
) {
    if detect_range.0 <= 0.0 {
        return;
    }

    let scouts: Vec<EntitySnapshot> = cache
        .snapshots
        .iter()
        .filter(|item| item.role == Role::Scout)
        .cloned()
        .collect();

    // 斥候ごとに探知対象を計算し、前回との差分で発見/失探を出力します。
    for scout in scouts {
        let mut detect_state = match detect_query.get_mut(scout.entity) {
            Ok(state) => state,
            Err(_) => continue,
        };

        let previous_state = detect_state.0.clone();
        let previous_ids: HashSet<String> = previous_state.keys().cloned().collect();
        let mut current_detected: HashMap<String, DetectionInfo> = HashMap::new();

        let base_key = cell_key(scout.position, detect_range.0);
        for dx in -1..=1 {
            for dy in -1..=1 {
                for dz in -1..=1 {
                    let key = CellKey {
                        x: base_key.x + dx,
                        y: base_key.y + dy,
                        z: base_key.z + dz,
                    };
                    let Some(indices) = cache.spatial_hash.get(&key) else {
                        continue;
                    };

                    for &other_index in indices {
                        let other = &cache.snapshots[other_index];
                        if other.entity == scout.entity {
                            continue;
                        }

                        if other.team_id == scout.team_id {
                            continue;
                        }

                        // 探知範囲内かどうかを最終的に距離で判定します。
                        let distance = distance_ecef(scout.position, other.position);
                        if distance > detect_range.0 {
                            continue;
                        }

                        let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(other.position);
                        let info = DetectionInfo {
                            lat_deg,
                            lon_deg,
                            alt_m,
                            distance_m: distance.round() as i64,
                        };
                        current_detected.insert(other.id.clone(), info);
                    }
                }
            }
        }

        let current_ids: HashSet<String> = current_detected.keys().cloned().collect();

        for id in current_ids.difference(&previous_ids) {
            if let Some(info) = current_detected.get(id) {
                // 新規発見イベントを出力します。
                events.detection_events.push(DetectionEvent {
                    event_type: "detection".to_string(),
                    detection_action: "found".to_string(),
                    time_sec: time.time_sec,
                    scount_id: scout.id.clone(),
                    lat_deg: info.lat_deg,
                    lon_deg: info.lon_deg,
                    alt_m: info.alt_m,
                    distance_m: info.distance_m,
                    detect_id: id.clone(),
                });
            }
        }

        for id in previous_ids.difference(&current_ids) {
            if let Some(info) = previous_state.get(id) {
                // 前回は見えていたが今回は見えないので失探イベントを出力します。
                events.detection_events.push(DetectionEvent {
                    event_type: "detection".to_string(),
                    detection_action: "lost".to_string(),
                    time_sec: time.time_sec,
                    scount_id: scout.id.clone(),
                    lat_deg: info.lat_deg,
                    lon_deg: info.lon_deg,
                    alt_m: info.alt_m,
                    distance_m: info.distance_m,
                    detect_id: id.clone(),
                });
            }
        }

        detect_state.0 = current_detected;
    }
}

pub fn detonation_system(
    time: Res<SimTime>,
    bom_range: Res<BomRange>,
    mut query: Query<(
        &Role,
        &StartSec,
        &TotalDurationSec,
        &Id,
        &Ecef,
        &mut HasDetonated,
    )>,
    mut events: ResMut<EventBuffer>,
) {
    // 攻撃役が最終地点に到達した時点で1回だけ爆破イベントを出します。
    for (role, start_sec, total_duration, id, position, mut has_detonated) in query.iter_mut() {
        if *role != Role::Attacker {
            continue;
        }

        if has_detonated.0 {
            continue;
        }

        let end_time = start_sec.0 as f64 + total_duration.0;
        if time.time_sec as f64 >= end_time {
            let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(*position);
            events.detonation_events.push(DetonationEvent {
                event_type: "detonation".to_string(),
                time_sec: time.time_sec,
                attacker_id: id.0.clone(),
                lat_deg,
                lon_deg,
                alt_m,
                bom_range_m: bom_range.0,
            });
            has_detonated.0 = true;
        }
    }
}
