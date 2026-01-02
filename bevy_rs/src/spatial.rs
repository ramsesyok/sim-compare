use crate::geo::Ecef;
use std::collections::HashMap;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct CellKey {
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

pub fn build_spatial_hash(positions: &[Ecef], cell_size: f64) -> HashMap<CellKey, Vec<usize>> {
    let mut map: HashMap<CellKey, Vec<usize>> = HashMap::new();
    if cell_size <= 0.0 {
        return map;
    }

    // 各オブジェクトをセルに割り当て、同じセルの候補を高速に取得できるようにします。
    for (index, pos) in positions.iter().enumerate() {
        let key = cell_key(*pos, cell_size);
        map.entry(key).or_default().push(index);
    }

    map
}

pub fn cell_key(pos: Ecef, cell_size: f64) -> CellKey {
    let x = (pos.x / cell_size).floor() as i32;
    let y = (pos.y / cell_size).floor() as i32;
    let z = (pos.z / cell_size).floor() as i32;
    CellKey { x, y, z }
}
