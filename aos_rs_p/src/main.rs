mod geo;
mod log;
mod scenario;
mod sim;
mod spatial;
mod spawn;

use clap::Parser;
use pprof::ProfilerGuard;
use prost::Message;
use rayon::prelude::*;
use std::env;
use std::fs::File;
use std::io::{BufWriter, Write};
use std::path::PathBuf;
use std::time::Instant;

#[derive(Parser, Debug)]
#[command(name = "aos_rs_p")]
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
        // 並列化の粒度を大きくしてスレッド起動の負担を減らすため、
        // 連続した要素を一定数ずつのチャンクに分けて処理します。
        // 環境変数でチャンクサイズを変えられるようにして計測しやすくします。
        let chunk_size = env::var("CHUNK_SIZE")
            .ok()
            .and_then(|value| value.parse::<usize>().ok())
            .filter(|value| *value > 0)
            .unwrap_or(256);
        objects.par_chunks_mut(chunk_size).for_each(|chunk| {
            for object in chunk {
                object.position_ecef = sim::position_at_time(object, time_sec as f64);
            }
        });

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

    let sim_elapsed = sim_start.elapsed();
    eprintln!("aos_rs: simulation elapsed: {:?}", sim_elapsed);

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
