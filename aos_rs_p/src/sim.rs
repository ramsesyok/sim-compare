use crate::geo::{distance_ecef, ecef_to_geodetic, Ecef};
use crate::log::{write_ndjson, DetectionEvent, DetonationEvent};
use crate::scenario::Role;
use crate::spatial::{cell_key, CellKey};
use rayon::prelude::*;
use std::collections::{HashMap, HashSet};
use std::fs::File;
use std::io::BufWriter;

// 経路上の1点を保持します（緯度経度高度＋ECEF座標）。
// 経路の補間やデバッグ確認に使うため保持しています。
#[derive(Debug)]
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

// AoSの1オブジェクト分の状態をまとめた構造体です。
// ID、役割、経路、現在位置、探知状態などを1つに集約します。
#[derive(Debug)]
pub struct ObjectState {
    pub id: String,
    pub team_id: String,
    pub role: Role,
    pub start_sec: i64,
    pub route: Vec<RoutePoint>,
    pub segment_end_secs: Vec<f64>,
    pub total_duration_sec: f64,
    pub position_ecef: Ecef,
    pub detect_state: HashMap<String, DetectionInfo>,
    pub has_detonated: bool,
}

// 並列処理で安全に読み取るための、オブジェクトの軽量スナップショットです。
// 参照先の共有や可変借用を避け、初心者でも理解しやすい形にしています。
#[derive(Clone)]
struct ObjectSnapshot {
    id: String,
    team_id: String,
    role: Role,
    position_ecef: Ecef,
}

// 探知イベントで使う、相手の位置と距離のスナップショットです。
// 「発見」「失探」の判定に利用します。
#[derive(Debug, Clone)]
pub struct DetectionInfo {
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub distance_m: i64,
}

pub fn position_at_time(object: &ObjectState, time_sec: f64) -> Ecef {
    // 司令官は移動しないので、最初の経路点に固定します。
    if object.role == Role::Commander {
        return object.route.first().map(|p| p.ecef).unwrap_or(Ecef {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        });
    }

    if object.route.is_empty() {
        return Ecef {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        };
    }

    // 移動開始前は最初の経路点に待機します。
    if time_sec < object.start_sec as f64 {
        return object.route[0].ecef;
    }

    // 経路が1点だけのときは、その位置に固定します。
    if object.segment_end_secs.is_empty() {
        return object.route[object.route.len() - 1].ecef;
    }

    // 全区間を移動し終えた場合は最終点に固定します。
    let elapsed = time_sec - object.start_sec as f64;
    if elapsed >= object.total_duration_sec {
        return object.route[object.route.len() - 1].ecef;
    }

    // 経過時間から現在の区間を探し、線形補間で位置を計算します。
    let mut segment_index = 0;
    while segment_index < object.segment_end_secs.len()
        && elapsed > object.segment_end_secs[segment_index]
    {
        segment_index += 1;
    }

    if segment_index >= object.segment_end_secs.len() {
        return object.route[object.route.len() - 1].ecef;
    }

    let segment_end = object.segment_end_secs[segment_index];
    let segment_start = if segment_index == 0 {
        0.0
    } else {
        object.segment_end_secs[segment_index - 1]
    };
    let segment_duration = segment_end - segment_start;
    if segment_duration <= 0.0 {
        return object.route[segment_index + 1].ecef;
    }

    let t = (elapsed - segment_start) / segment_duration;
    let a = object.route[segment_index].ecef;
    let b = object.route[segment_index + 1].ecef;

    Ecef {
        x: a.x + (b.x - a.x) * t,
        y: a.y + (b.y - a.y) * t,
        z: a.z + (b.z - a.z) * t,
    }
}

pub fn emit_detection_events(
    time_sec: i64,
    detect_range_m: f64,
    spatial_hash: &HashMap<CellKey, Vec<usize>>,
    objects: &mut [ObjectState],
    event_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    if detect_range_m <= 0.0 {
        return Ok(());
    }

    // 斥候ごとに探知対象を計算し、前回との差分で発見/失探を出力します。
    // 並列化によって探知判定の重い部分を分散しますが、ログ出力は順序を保つために直列で行います。
    let snapshots: Vec<ObjectSnapshot> = objects
        .iter()
        .map(|object| ObjectSnapshot {
            id: object.id.clone(),
            team_id: object.team_id.clone(),
            role: object.role,
            position_ecef: object.position_ecef,
        })
        .collect();
    let previous_detect_states: Vec<Option<HashMap<String, DetectionInfo>>> = objects
        .iter()
        .map(|object| {
            if object.role == Role::Scout {
                Some(object.detect_state.clone())
            } else {
                None
            }
        })
        .collect();

    struct DetectionWorkResult {
        scout_index: usize,
        current_detected: HashMap<String, DetectionInfo>,
        events: Vec<DetectionEvent>,
    }

    let mut results: Vec<DetectionWorkResult> = (0..objects.len())
        .into_par_iter()
        .filter_map(|i| {
            let scout = &snapshots[i];
            if scout.role != Role::Scout {
                return None;
            }

            let scout_pos = scout.position_ecef;
            let scout_team = &scout.team_id;
            let scout_id = &scout.id;

            let mut current_detected: HashMap<String, DetectionInfo> = HashMap::new();

            let base_key = cell_key(scout_pos, detect_range_m);
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
                            if other_index == i {
                                continue;
                            }

                            let other = &snapshots[other_index];
                            if other.team_id == *scout_team {
                                continue;
                            }

                            // 探知範囲内かどうかを最終的に距離で判定します。
                            let distance = distance_ecef(scout_pos, other.position_ecef);
                            if distance > detect_range_m {
                                continue;
                            }

                            let (lat_deg, lon_deg, alt_m) =
                                ecef_to_geodetic(other.position_ecef);
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

            let previous_detect_state = previous_detect_states[i].as_ref().expect("scout");
            let previous_ids: HashSet<String> = previous_detect_state.keys().cloned().collect();
            let current_ids: HashSet<String> = current_detected.keys().cloned().collect();
            let mut events = Vec::new();

            for id in current_ids.difference(&previous_ids) {
                if let Some(info) = current_detected.get(id) {
                    // 新規発見イベントを出力します。
                    events.push(DetectionEvent {
                        event_type: "detection".to_string(),
                        detection_action: "found".to_string(),
                        time_sec,
                        scount_id: scout_id.clone(),
                        lat_deg: info.lat_deg,
                        lon_deg: info.lon_deg,
                        alt_m: info.alt_m,
                        distance_m: info.distance_m,
                        detect_id: id.clone(),
                    });
                }
            }

            for id in previous_ids.difference(&current_ids) {
                if let Some(info) = previous_detect_state.get(id) {
                    // 前回は見えていたが今回は見えないので失探イベントを出力します。
                    events.push(DetectionEvent {
                        event_type: "detection".to_string(),
                        detection_action: "lost".to_string(),
                        time_sec,
                        scount_id: scout_id.clone(),
                        lat_deg: info.lat_deg,
                        lon_deg: info.lon_deg,
                        alt_m: info.alt_m,
                        distance_m: info.distance_m,
                        detect_id: id.clone(),
                    });
                }
            }

            Some(DetectionWorkResult {
                scout_index: i,
                current_detected,
                events,
            })
        })
        .collect();

    results.sort_by_key(|result| result.scout_index);
    for result in results {
        for event in result.events {
            write_ndjson(event_writer, &event)?;
        }
        objects[result.scout_index].detect_state = result.current_detected;
    }

    Ok(())
}

pub fn emit_detonation_events(
    time_sec: i64,
    bom_range_m: i64,
    objects: &mut [ObjectState],
    event_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    // 攻撃役が最終地点に到達した時点で1回だけ爆破イベントを出します。
    for object in objects.iter_mut() {
        if object.role != Role::Attacker {
            continue;
        }

        if object.has_detonated {
            continue;
        }

        let end_time = object.start_sec as f64 + object.total_duration_sec;
        if time_sec as f64 >= end_time {
            let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(object.position_ecef);
            let event = DetonationEvent {
                event_type: "detonation".to_string(),
                time_sec,
                attacker_id: object.id.clone(),
                lat_deg,
                lon_deg,
                alt_m,
                bom_range_m,
            };
            write_ndjson(event_writer, &event)?;
            object.has_detonated = true;
        }
    }

    Ok(())
}
