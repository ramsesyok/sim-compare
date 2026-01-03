package main

import "math"

type Ecef struct {
	X float64
	Y float64
	Z float64
}

func distanceECEF(a, b Ecef) float64 {
	// ECEF空間でのユークリッド距離を計算します。
	dx := a.X - b.X
	dy := a.Y - b.Y
	dz := a.Z - b.Z
	return math.Sqrt(dx*dx + dy*dy + dz*dz)
}

func geodeticToECEF(latDeg, lonDeg, altM float64) Ecef {
	// WGS84の測地座標をECEF座標へ変換します。
	lat := latDeg * math.Pi / 180.0
	lon := lonDeg * math.Pi / 180.0

	a := 6378137.0
	f := 1.0 / 298.257223563
	e2 := f * (2.0 - f)

	sinLat := math.Sin(lat)
	cosLat := math.Cos(lat)
	sinLon := math.Sin(lon)
	cosLon := math.Cos(lon)

	n := a / math.Sqrt(1.0-e2*sinLat*sinLat)

	return Ecef{
		X: (n + altM) * cosLat * cosLon,
		Y: (n + altM) * cosLat * sinLon,
		Z: (n*(1.0-e2) + altM) * sinLat,
	}
}

func ecefToGeodetic(pos Ecef) (float64, float64, float64) {
	// ECEF座標をWGS84の緯度経度高度に戻します（反復近似）。
	a := 6378137.0
	f := 1.0 / 298.257223563
	e2 := f * (2.0 - f)

	p := math.Sqrt(pos.X*pos.X + pos.Y*pos.Y)
	lat := math.Atan2(pos.Z, p)
	lon := math.Atan2(pos.Y, pos.X)
	alt := 0.0

	for i := 0; i < 5; i++ {
		sinLat := math.Sin(lat)
		n := a / math.Sqrt(1.0-e2*sinLat*sinLat)
		alt = p/math.Cos(lat) - n
		lat = math.Atan2(pos.Z, p*(1.0-e2*n/(n+alt)))
		lon = math.Atan2(pos.Y, pos.X)
	}

	return lat * 180.0 / math.Pi, lon * 180.0 / math.Pi, alt
}
