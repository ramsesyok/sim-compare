mod geo;
mod log;
mod scenario;
mod sim;
mod spatial;
mod spawn;

use bevy_ecs::schedule::{IntoSystemConfigs, Schedule};
use clap::Parser;
use std::fs::File;
use std::io::BufWriter;
use std::path::PathBuf;
use std::time::Instant;

#[derive(Parser, Debug)]
#[command(name = "bevy_rs")]
struct Cli {
    #[arg(long)]
    scenario: PathBuf,
    #[arg(long)]
    timeline_log: PathBuf,
    #[arg(long)]
    event_log: PathBuf,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let cli = Cli::parse();

    // シナリオJSONを読み込み、シミュレーションの元データを構築します。
    let scenario = scenario::load_scenario(&cli.scenario)?;

    // Bevy ECSでは「エンティティ＋コンポーネント」で状態を管理します。
    let mut world = spawn::build_world(&scenario);

    world.insert_resource(sim::SimTime { time_sec: 0 });
    world.insert_resource(sim::DetectRange(
        scenario.performance.scout.detect_range_m as f64,
    ));
    world.insert_resource(sim::BomRange(scenario.performance.attacker.bom_range_m));
    world.insert_resource(sim::SnapshotCache::default());
    world.insert_resource(sim::EventBuffer::default());
    world.insert_resource(log::TimelineBuffer::default());

    let mut schedule = Schedule::default();
    schedule.add_systems(
        (
            sim::position_update_system,
            sim::snapshot_system,
            sim::detection_system,
            sim::detonation_system,
            log::timeline_system,
        )
            .chain(),
    );

    let timeline_file = File::create(cli.timeline_log)?;
    let event_file = File::create(cli.event_log)?;
    let mut timeline_writer = BufWriter::new(timeline_file);
    let mut event_writer = BufWriter::new(event_file);

    let end_sec = 24 * 60 * 60;

    // Bevy ECSではSystemとして処理を分割し、Scheduleで順番を明示します。
    // ここではリアルタイム実行は行わず、forループで秒刻みの処理を高速に回すだけです。
    // シミュレーション全体の処理時間を計測します（ログ出力完了まで）。
    let sim_start = Instant::now();

    for time_sec in 0..=end_sec {
        // Systemから参照される時刻リソースを更新します。
        if let Some(mut time) = world.get_resource_mut::<sim::SimTime>() {
            time.time_sec = time_sec;
        }

        // Scheduleは単にシステム群の実行順を管理するだけで、待機やフレーム制御はありません。
        schedule.run(&mut world);

        // Systemが溜めたイベントをログへ書き出します。
        {
            let mut event_buffer = world.resource_mut::<sim::EventBuffer>();
            for event in event_buffer.detection_events.drain(..) {
                log::write_ndjson(&mut event_writer, &event)?;
            }
            for event in event_buffer.detonation_events.drain(..) {
                log::write_ndjson(&mut event_writer, &event)?;
            }
        }

        // Systemが溜めたタイムラインをログへ書き出します。
        {
            let mut timeline_buffer = world.resource_mut::<log::TimelineBuffer>();
            for entry in timeline_buffer.logs.drain(..) {
                log::write_ndjson(&mut timeline_writer, &entry)?;
            }
        }
    }

    let sim_elapsed = sim_start.elapsed();
    eprintln!("bevy_rs: simulation elapsed: {:?}", sim_elapsed);

    Ok(())
}
