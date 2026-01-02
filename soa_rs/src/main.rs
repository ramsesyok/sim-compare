mod geo;
mod log;
mod scenario;
mod sim;
mod spatial;
mod spawn;

use clap::Parser;
use pprof::ProfilerGuard;
use prost::Message;
use std::fs::File;
use std::io::{BufWriter, Write};
use std::path::PathBuf;
use std::time::Instant;

#[derive(Parser, Debug)]
#[command(name = "soa_rs")]
struct Cli {
    #[arg(long)]
    scenario: PathBuf,
    #[arg(long)]
    timeline_log: PathBuf,
    #[arg(long)]
    event_log: PathBuf,
    #[arg(long)]
    cpu_profile: Option<PathBuf>,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let cli = Cli::parse();

    // シナリオJSONを読み込み、シミュレーションの元データを構築します。
    let scenario = scenario::load_scenario(&cli.scenario)?;

    // SoAなので、同じ種類のデータを配列でまとめた状態を作ります。
    let mut state = spawn::build_state(&scenario);

    let timeline_file = File::create(cli.timeline_log)?;
    let event_file = File::create(cli.event_log)?;
    let mut timeline_writer = BufWriter::new(timeline_file);
    let mut event_writer = BufWriter::new(event_file);

    let detect_range_m = scenario.performance.scout.detect_range_m as f64;
    let bom_range_m = scenario.performance.attacker.bom_range_m;

    let end_sec = 24 * 60 * 60;

    // SoAは「属性ごとに配列を持つ」設計で、
    // ここでは位置配列・役割配列・経路配列などを別々に管理しています。
    // 初心者でも追いやすいように、毎秒すべての要素を順番に更新しています。
    // シミュレーション全体の処理時間を計測します（ログ出力完了まで）。
    let sim_start = Instant::now();
    // CPUプロファイルは「処理中にどこで時間を使っているか」を調べるための仕組みなので、
    // 指定されたときだけ開始し、最後にまとめて書き出します。
    let cpu_guard = if cli.cpu_profile.is_some() {
        Some(ProfilerGuard::new(100)?)
    } else {
        None
    };

    for time_sec in 0..=end_sec {
        // まず全オブジェクトの位置を更新します。
        sim::update_positions(&mut state, time_sec as f64);

        // 空間ハッシュを作り、近傍探索を高速化します。
        let spatial_hash = spatial::build_spatial_hash(&state.positions, detect_range_m);

        // 斥候の探知イベントを判定してイベントログへ出力します。
        sim::emit_detection_events(
            time_sec,
            detect_range_m,
            &spatial_hash,
            &mut state,
            &mut event_writer,
        )?;

        // 攻撃役の爆破イベントを判定してイベントログへ出力します。
        sim::emit_detonation_events(time_sec, bom_range_m, &mut state, &mut event_writer)?;

        // 全オブジェクトの位置をタイムラインログへ出力します。
        log::emit_timeline_log(
            time_sec,
            &state.ids,
            &state.team_ids,
            &state.roles,
            &state.positions,
            &mut timeline_writer,
        )?;
    }

    let sim_elapsed = sim_start.elapsed();
    eprintln!("soa_rs: simulation elapsed: {:?}", sim_elapsed);

    // CPUプロファイルの出力は、計測が終わってから1回だけまとめて行います。
    if let (Some(path), Some(guard)) = (cli.cpu_profile, cpu_guard) {
        let report = guard.report().build()?;
        let profile = report.pprof()?;
        let mut file = File::create(path)?;
        let encoded = profile.encode_to_vec();
        file.write_all(&encoded)?;
    }

    Ok(())
}
