use crate::geo::ecef_to_geodetic;
use crate::scenario::Role;
use bevy_ecs::entity::Entity;
use bevy_ecs::world::World;
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
    world: &World,
    entities: &[Entity],
    timeline_writer: &mut BufWriter<File>,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut positions_log = Vec::with_capacity(entities.len());

    // 1秒ごとの全オブジェクト位置をまとめて1行に出力します。
    for &entity in entities {
        let id = world.get::<crate::sim::Id>(entity).ok_or("id missing")?;
        let team_id = world.get::<crate::sim::TeamId>(entity).ok_or("team_id missing")?;
        let role = world.get::<Role>(entity).ok_or("role missing")?;
        let position = world.get::<crate::geo::Ecef>(entity).ok_or("position missing")?;
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

    let log = TimelineLog {
        time_sec,
        positions: positions_log,
    };
    write_ndjson(timeline_writer, &log)?;

    Ok(())
}
