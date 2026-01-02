use crate::geo::ecef_to_geodetic;
use crate::geo::Ecef;
use crate::scenario::Role;
use crate::sim::{Id, SimTime, TeamId};
use bevy_ecs::prelude::{Query, Res, ResMut, Resource};
use serde::Serialize;
use std::fs::File;
use std::io::{BufWriter, Write};

#[derive(Debug, Serialize)]
pub struct TimelineLog {
    pub time_sec: i64,
    pub positions: Vec<TimelinePosition>,
}

#[derive(Debug, Serialize)]
pub struct TimelinePosition {
    pub object_id: String,
    pub team_id: String,
    pub role: String,
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
}

#[derive(Debug, Serialize)]
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

#[derive(Debug, Serialize)]
pub struct DetonationEvent {
    pub event_type: String,
    pub time_sec: i64,
    pub attacker_id: String,
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub bom_range_m: i64,
}

#[derive(Debug, Default, Resource)]
pub struct TimelineBuffer {
    pub logs: Vec<TimelineLog>,
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

pub fn timeline_system(
    time: Res<SimTime>,
    mut timeline: ResMut<TimelineBuffer>,
    query: Query<(&Id, &TeamId, &Role, &Ecef)>,
) {
    let mut positions_log = Vec::with_capacity(query.iter().count());

    // 1秒ごとの全オブジェクト位置をまとめて1行に出力します。
    for (id, team_id, role, position) in query.iter() {
        let (lat_deg, lon_deg, alt_m) = ecef_to_geodetic(*position);
        positions_log.push(TimelinePosition {
            object_id: id.0.clone(),
            team_id: team_id.0.clone(),
            role: role.as_str().to_string(),
            lat_deg,
            lon_deg,
            alt_m,
        });
    }

    timeline.logs.push(TimelineLog {
        time_sec: time.time_sec,
        positions: positions_log,
    });
}
