mod geo;
mod log;
mod scenario;
mod sim;
mod spatial;
mod spawn;

use clap::Parser;
use std::fs::File;
use std::io::BufWriter;
use std::path::PathBuf;

#[derive(Parser, Debug)]
#[command(name = "aos_rs")]
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

    // AoSなので、オブジェクト単位で状態をまとめた配列を作ります。
    let mut objects = spawn::build_objects(&scenario);

    let timeline_file = File::create(cli.timeline_log)?;
    let event_file = File::create(cli.event_log)?;
    let mut timeline_writer = BufWriter::new(timeline_file);
    let mut event_writer = BufWriter::new(event_file);

    let detect_range_m = scenario.performance.scout.detect_range_m as f64;
    let bom_range_m = scenario.performance.attacker.bom_range_m;

    let end_sec = 24 * 60 * 60;

    // AoSは「1つのオブジェクトに必要な情報をまとめて持つ」設計で、
    // ここでは各オブジェクトの状態（位置・経路・検知履歴など）を1つの構造体に詰めています。
    // 初心者でも追いやすいように、毎秒すべてのオブジェクトを順番に更新しています。
    for time_sec in 0..=end_sec {
        // まず全オブジェクトの位置を更新します。
        for object in objects.iter_mut() {
            object.position_ecef = sim::position_at_time(object, time_sec as f64);
        }

        // 空間ハッシュを作り、近傍探索を高速化します。
        let spatial_hash = spatial::build_spatial_hash(&objects, detect_range_m);

        // 斥候の探知イベントを判定してイベントログへ出力します。
        sim::emit_detection_events(
            time_sec,
            detect_range_m,
            &spatial_hash,
            &mut objects,
            &mut event_writer,
        )?;

        // 攻撃役の爆破イベントを判定してイベントログへ出力します。
        sim::emit_detonation_events(time_sec, bom_range_m, &mut objects, &mut event_writer)?;

        // 全オブジェクトの位置をタイムラインログへ出力します。
        log::emit_timeline_log(time_sec, &objects, &mut timeline_writer)?;
    }

    Ok(())
}
