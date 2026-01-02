use crate::geo::ecef_to_geodetic;
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
    objects: &[crate::sim::ObjectState],
    timeline_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut positions_log = Vec::with_capacity(objects.len());

    // 1秒ごとの全オブジェクト位置をまとめて1行に出力します。
    for object in objects {
        let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(object.position_ecef);
        positions_log.push(TimelinePosition {
            object_id: object.id.clone(),
            team_id: object.team_id.clone(),
            role: object.role.as_str().to_string(),
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
