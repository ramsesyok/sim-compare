use crate::geo::{distance_ecef, ecef_to_geodetic, Ecef};
use crate::log::{write_ndjson, DetectionEvent, DetonationEvent};
use crate::scenario::Role;
use crate::spatial::{cell_key, CellKey};
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

// 探知イベントで使う、相手の位置と距離のスナップショットです。
// 「発見」「失探」の判定に利用します。
#[derive(Debug, Clone)]
pub struct DetectionInfo {
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub distance_m: i64,
}

// SoAの状態を属性ごとの配列で保持します。
// 同じ種類のデータを並べ、アクセスパターンを明確にします。
#[derive(Debug)]
pub struct SoaState {
    pub ids: Vec<String>,
    pub team_ids: Vec<String>,
    pub roles: Vec<Role>,
    pub start_secs: Vec<i64>,
    pub routes: Vec<Vec<RoutePoint>>,
    pub segment_end_secs: Vec<Vec<f64>>,
    pub total_duration_secs: Vec<f64>,
    pub positions: Vec<Ecef>,
    pub detect_state: Vec<HashMap<String, DetectionInfo>>,
    pub has_detonated: Vec<bool>,
}

pub fn update_positions(state: &mut SoaState, time_sec: f64) {
    for i in 0..state.ids.len() {
        state.positions[i] = position_at_time(state, i, time_sec);
    }
}

fn position_at_time(state: &SoaState, index: usize, time_sec: f64) -> Ecef {
    // 司令官は移動しないので、最初の経路点に固定します。
    if state.roles[index] == Role::Commander {
        return state
            .routes
            .get(index)
            .and_then(|route| route.first())
            .map(|p| p.ecef)
            .unwrap_or(Ecef {
                x: 0.0,
                y: 0.0,
                z: 0.0,
            });
    }

    if state.routes[index].is_empty() {
        return Ecef {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        };
    }

    // 移動開始前は最初の経路点に待機します。
    if time_sec < state.start_secs[index] as f64 {
        return state.routes[index][0].ecef;
    }

    // 経路が1点だけのときは、その位置に固定します。
    if state.segment_end_secs[index].is_empty() {
        return state.routes[index][state.routes[index].len() - 1].ecef;
    }

    // 全区間を移動し終えた場合は最終点に固定します。
    let elapsed = time_sec - state.start_secs[index] as f64;
    if elapsed >= state.total_duration_secs[index] {
        return state.routes[index][state.routes[index].len() - 1].ecef;
    }

    // 経過時間から現在の区間を探し、線形補間で位置を計算します。
    let mut segment_index = 0;
    while segment_index < state.segment_end_secs[index].len()
        && elapsed > state.segment_end_secs[index][segment_index]
    {
        segment_index += 1;
    }

    if segment_index >= state.segment_end_secs[index].len() {
        return state.routes[index][state.routes[index].len() - 1].ecef;
    }

    let segment_end = state.segment_end_secs[index][segment_index];
    let segment_start = if segment_index == 0 {
        0.0
    } else {
        state.segment_end_secs[index][segment_index - 1]
    };
    let segment_duration = segment_end - segment_start;
    if segment_duration <= 0.0 {
        return state.routes[index][segment_index + 1].ecef;
    }

    let t = (elapsed - segment_start) / segment_duration;
    let a = state.routes[index][segment_index].ecef;
    let b = state.routes[index][segment_index + 1].ecef;

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
    state: &mut SoaState,
    event_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    if detect_range_m <= 0.0 {
        return Ok(());
    }

    // 斥候ごとに探知対象を計算し、前回との差分で発見/失探を出力します。
    for i in 0..state.ids.len() {
        if state.roles[i] != Role::Scout {
            continue;
        }

        let scout_pos = state.positions[i];
        let scout_team = state.team_ids[i].clone();
        let scout_id = state.ids[i].clone();

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

                        if state.team_ids[other_index] == scout_team {
                            continue;
                        }

                        // 探知範囲内かどうかを最終的に距離で判定します。
                        let distance = distance_ecef(scout_pos, state.positions[other_index]);
                        if distance > detect_range_m {
                            continue;
                        }

                        let (lat_deg, lon_deg, alt_m) =
                            ecef_to_geodetic(state.positions[other_index]);
                        let info = DetectionInfo {
                            lat_deg,
                            lon_deg,
                            alt_m,
                            distance_m: distance.round() as i64,
                        };
                        current_detected.insert(state.ids[other_index].clone(), info);
                    }
                }
            }
        }

        let previous_ids: HashSet<String> = state.detect_state[i].keys().cloned().collect();
        let current_ids: HashSet<String> = current_detected.keys().cloned().collect();

        for id in current_ids.difference(&previous_ids) {
            if let Some(info) = current_detected.get(id) {
                // 新規発見イベントを出力します。
                let event = DetectionEvent {
                    event_type: "detection".to_string(),
                    detection_action: "found".to_string(),
                    time_sec,
                    scount_id: scout_id.clone(),
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
            if let Some(info) = state.detect_state[i].get(id) {
                // 前回は見えていたが今回は見えないので失探イベントを出力します。
                let event = DetectionEvent {
                    event_type: "detection".to_string(),
                    detection_action: "lost".to_string(),
                    time_sec,
                    scount_id: scout_id.clone(),
                    lat_deg: info.lat_deg,
                    lon_deg: info.lon_deg,
                    alt_m: info.alt_m,
                    distance_m: info.distance_m,
                    detect_id: id.clone(),
                };
                write_ndjson(event_writer, &event)?;
            }
        }

        state.detect_state[i] = current_detected;
    }

    Ok(())
}

pub fn emit_detonation_events(
    time_sec: i64,
    bom_range_m: i64,
    state: &mut SoaState,
    event_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    // 攻撃役が最終地点に到達した時点で1回だけ爆破イベントを出します。
    for i in 0..state.ids.len() {
        if state.roles[i] != Role::Attacker {
            continue;
        }

        if state.has_detonated[i] {
            continue;
        }

        let end_time = state.start_secs[i] as f64 + state.total_duration_secs[i];
        if time_sec as f64 >= end_time {
            let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(state.positions[i]);
            let event = DetonationEvent {
                event_type: "detonation".to_string(),
                time_sec,
                attacker_id: state.ids[i].clone(),
                lat_deg,
                lon_deg,
                alt_m,
                bom_range_m,
            };
            write_ndjson(event_writer, &event)?;
            state.has_detonated[i] = true;
        }
    }

    Ok(())
}
