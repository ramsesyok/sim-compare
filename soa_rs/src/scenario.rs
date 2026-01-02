use serde::Deserialize;
use std::fs::File;
use std::path::Path;

#[derive(Debug, Deserialize)]
#[allow(dead_code)]
pub struct Scenario {
    // 今後の通信・探知・攻撃仕様の拡張で参照するため保持しています。
    pub performance: Performance,
    pub teams: Vec<Team>,
}

#[derive(Debug, Deserialize)]
#[allow(dead_code)]
pub struct Performance {
    // 今後の通信・探知・攻撃仕様の拡張で参照するため保持しています。
    pub scout: ScoutPerformance,
    // 伝令役の通信仕様を将来の機能で利用する予定です。
    pub messenger: MessengerPerformance,
    // 攻撃役の爆破仕様を将来の機能で利用する予定です。
    pub attacker: AttackerPerformance,
}

#[derive(Debug, Deserialize)]
#[allow(dead_code)]
pub struct ScoutPerformance {
    // 通信機能の実装時に使用予定です。
    pub comm_range_m: i64,
    pub detect_range_m: i64,
}

#[derive(Debug, Deserialize)]
#[allow(dead_code)]
pub struct MessengerPerformance {
    // 伝令役の通信判定に使う予定です。
    pub comm_range_m: i64,
}

#[derive(Debug, Deserialize)]
pub struct AttackerPerformance {
    pub bom_range_m: i64,
}

#[derive(Debug, Deserialize)]
#[allow(dead_code)]
pub struct Team {
    pub id: String,
    // 将来、ログや可視化で利用するため保持しています。
    pub name: String,
    pub objects: Vec<ObjectScenario>,
}

#[derive(Debug, Deserialize)]
#[allow(dead_code)]
pub struct ObjectScenario {
    pub id: String,
    pub role: Role,
    pub start_sec: i64,
    pub route: Vec<Waypoint>,
    // 通信経路の仕様を将来実装するため保持しています。
    #[serde(default)]
    pub network: Vec<String>,
}

#[derive(Debug, Deserialize, Clone, Copy, PartialEq, Eq)]
#[serde(rename_all = "lowercase")]
pub enum Role {
    Commander,
    Scout,
    Messenger,
    Attacker,
}

impl Role {
    pub fn as_str(&self) -> &'static str {
        match self {
            Role::Commander => "commander",
            Role::Scout => "scout",
            Role::Messenger => "messenger",
            Role::Attacker => "attacker",
        }
    }
}

#[derive(Debug, Deserialize, Clone)]
pub struct Waypoint {
    pub lat_deg: f64,
    pub lon_deg: f64,
    pub alt_m: f64,
    pub speeds_kph: f64,
}

pub fn load_scenario(path: &Path) -> Result<Scenario, Box<dyn std::error::Error>> {
    // シナリオJSONを読み込み、シミュレーションの元データを構築します。
    let scenario_file = File::open(path)?;
    let scenario: Scenario = serde_json::from_reader(scenario_file)?;
    Ok(scenario)
}
