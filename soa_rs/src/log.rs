use crate::geo::ecef_to_geodetic;
use crate::scenario::Role;
use serde::Serialize;
use std::fs::File;
use std::io::{BufWriter, Write};

#[derive(Serialize)]
pub struct TimelineLog {
    pub time_sec: i64,
    pub positions: Vec<TimelinePosition>,
}

#[derive(Serialize)]
pub struct TimelinePosition {
    pub object_id: String,
    pub team_id: String,
    pub role: String,
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
}

#[derive(Serialize)]
pub struct DetectionEvent {
    pub event_type: String,
    pub detection_action: String,
    pub time_sec: i64,
    pub scount_id: String,
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub distance_m: i64,
    pub detect_id: String,
}

#[derive(Serialize)]
pub struct DetonationEvent {
    pub event_type: String,
    pub time_sec: i64,
    pub attacker_id: String,
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub bom_range_m: i64,
}

pub fn write_ndjson<T: Serialize>(
    writer: &mut BufWriter<File>,
    value: &T,
) -> Result<(), Box<dyn std::error::Error>> {
    let line = serde_json::to_string(value)?;
    writer.write_all(line.as_bytes())?;
    writer.write_all(b"\n")?;
    Ok(())
}

pub fn emit_timeline_log(
    time_sec: i64,
    ids: &[String],
    team_ids: &[String],
    roles: &[Role],
    positions: &[crate::geo::Ecef],
    timeline_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut positions_log = Vec::with_capacity(ids.len());

    // 1秒ごとの全オブジェクト位置をまとめて1行に出力します。
    for i in 0..ids.len() {
        let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(positions[i]);
        positions_log.push(TimelinePosition {
            object_id: ids[i].clone(),
            team_id: team_ids[i].clone(),
            role: roles[i].as_str().to_string(),
            lat_deg,
            lon_deg,
            alt_m,
        });
    }

    let log = TimelineLog {
        time_sec,
        positions: positions_log,
    };
    write_ndjson(timeline_writer, &log)?;

    Ok(())
}
