use crate::geo::{distance_ecef, ecef_to_geodetic, Ecef};
use crate::log::{write_ndjson, DetectionEvent, DetonationEvent};
use crate::scenario::Role;
use crate::spatial::{cell_key, CellKey};
use hecs::World;
use std::collections::{HashMap, HashSet};
use std::fs::File;
use std::io::BufWriter;

#[derive(Debug, Clone)]
pub struct Id(pub String);

#[derive(Debug, Clone)]
pub struct TeamId(pub String);

#[derive(Debug, Clone)]
pub struct StartSec(pub i64);

#[derive(Debug, Clone)]
pub struct SegmentEndSecs(pub Vec<f64>);

#[derive(Debug, Clone)]
pub struct TotalDurationSec(pub f64);

#[derive(Debug, Clone, Default)]
pub struct DetectState(pub HashMap<String, DetectionInfo>);

#[derive(Debug, Clone)]
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

#[derive(Debug, Clone)]
pub struct RoutePoints(pub Vec<RoutePoint>);

#[derive(Debug, Clone)]
pub struct DetectionInfo {
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub distance_m: i64,
}

pub fn update_positions(
    world: &mut World,
    time_sec: f64,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut query = world.query::<(
        &Role,
        &StartSec,
        &RoutePoints,
        &SegmentEndSecs,
        &TotalDurationSec,
        &mut Ecef,
    )>();

    for (_entity, (role, start_sec, route, segment_end, total_duration, position)) in query.iter() {
        *position = position_at_time(
            *role,
            start_sec.0,
            &route.0,
            &segment_end.0,
            total_duration.0,
            time_sec,
        );
    }

    Ok(())
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

pub fn collect_positions_and_meta(
    world: &World,
) -> Result<Vec<EntitySnapshot>, Box<dyn std::error::Error>> {
    let mut snapshots = Vec::new();
    let mut query = world.query::<(&Id, &TeamId, &Role, &Ecef)>();
    for (entity, (id, team_id, role, position)) in query.iter() {
        snapshots.push(EntitySnapshot {
            entity,
            id: id.0.clone(),
            team_id: team_id.0.clone(),
            role: *role,
            position: *position,
        });
    }
    Ok(snapshots)
}

#[derive(Clone)]
pub struct EntitySnapshot {
    pub entity: hecs::Entity,
    pub id: String,
    pub team_id: String,
    pub role: Role,
    pub position: Ecef,
}

pub fn emit_detection_events(
    time_sec: i64,
    detect_range_m: f64,
    spatial_hash: &HashMap<CellKey, Vec<usize>>,
    snapshots: &[EntitySnapshot],
    world: &mut World,
    event_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    if detect_range_m <= 0.0 {
        return Ok(());
    }

    let scouts: Vec<EntitySnapshot> = snapshots
        .iter()
        .filter(|item| item.role == Role::Scout)
        .cloned()
        .collect();

    // 斥候ごとに探知対象を計算し、前回との差分で発見/失探を出力します。
    for scout in scouts {
        let mut current_detected: HashMap<String, DetectionInfo> = HashMap::new();

        let base_key = cell_key(scout.position, detect_range_m);
        for dx in -1..=1 {
            for dy in -1..=1 {
                for dz in -1..=1 {
                    let key = CellKey {
                        x: base_key.x + dx,
                        y: base_key.y + dy,
                        z: base_key.z + dz,
                    };
                    let Some(indices) = spatial_hash.get(&key) else {
                        continue;
                    };

                    for &other_index in indices {
                        let other = &snapshots[other_index];
                        if other.entity == scout.entity {
                            continue;
                        }

                        if other.team_id == scout.team_id {
                            continue;
                        }

                        // 探知範囲内かどうかを最終的に距離で判定します。
                        let distance = distance_ecef(scout.position, other.position);
                        if distance > detect_range_m {
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

        let previous_ids = {
            let detect_state = world.get::<&DetectState>(scout.entity)?;
            detect_state.0.keys().cloned().collect::<HashSet<String>>()
        };
        let current_ids: HashSet<String> = current_detected.keys().cloned().collect();

        for id in current_ids.difference(&previous_ids) {
            if let Some(info) = current_detected.get(id) {
                // 新規発見イベントを出力します。
                let event = DetectionEvent {
                    event_type: "detection".to_string(),
                    detection_action: "found".to_string(),
                    time_sec,
                    scount_id: scout.id.clone(),
                    lat_deg: info.lat_deg,
                    lon_deg: info.lon_deg,
                    alt_m: info.alt_m,
                    distance_m: info.distance_m,
                    detect_id: id.clone(),
                };
                write_ndjson(event_writer, &event)?;
            }
        }

        for id in previous_ids.difference(&current_ids) {
            let detect_state = world.get::<&DetectState>(scout.entity)?;
            if let Some(info) = detect_state.0.get(id) {
                // 前回は見えていたが今回は見えないので失探イベントを出力します。
                let event = DetectionEvent {
                    event_type: "detection".to_string(),
                    detection_action: "lost".to_string(),
                    time_sec,
                    scount_id: scout.id.clone(),
                    lat_deg: info.lat_deg,
                    lon_deg: info.lon_deg,
                    alt_m: info.alt_m,
                    distance_m: info.distance_m,
                    detect_id: id.clone(),
                };
                write_ndjson(event_writer, &event)?;
            }
        }

        if let Ok(mut detect_state) = world.get::<&mut DetectState>(scout.entity) {
            detect_state.0 = current_detected;
        }
    }

    Ok(())
}

pub fn emit_detonation_events(
    time_sec: i64,
    bom_range_m: i64,
    world: &mut World,
    event_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    // 攻撃役が最終地点に到達した時点で1回だけ爆破イベントを出します。
    let mut query = world.query::<(
        &Role,
        &StartSec,
        &TotalDurationSec,
        &Id,
        &Ecef,
        &mut HasDetonated,
    )>();
    for (_entity, (role, start_sec, total_duration, id, position, has_detonated)) in query.iter() {
        if *role != Role::Attacker {
            continue;
        }

        if has_detonated.0 {
            continue;
        }

        let end_time = start_sec.0 as f64 + total_duration.0;
        if time_sec as f64 >= end_time {
            let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(*position);
            let event = DetonationEvent {
                event_type: "detonation".to_string(),
                time_sec,
                attacker_id: id.0.clone(),
                lat_deg,
                lon_deg,
                alt_m,
                bom_range_m,
            };
            write_ndjson(event_writer, &event)?;
            has_detonated.0 = true;
        }
    }

    Ok(())
}
