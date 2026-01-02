echo "Running benchmarks with small scenario..."
./soa_rs/target/release/soa_rs --scenario ./scenario_small.json --timeline-log timeline_soa_rs.ndjson --event-log event_soa_rs_small.ndjson
./aos_rs/target/release/aos_rs --scenario ./scenario_small.json --timeline-log timeline_aos_rs_small.ndjson --event-log event_aos_rs_small.ndjson
./hecs_rs/target/release/hecs_rs --scenario ./scenario_small.json --timeline-log timeline_hecs_rs_small.ndjson --event-log event_hecs_rs_small.ndjson
./bevy_rs/target/release/bevy_rs --scenario ./scenario_small.json --timeline-log timeline_bevy_rs_small.ndjson --event-log event_bevy_rs_small.ndjson
./ark_go/ark_go --scenario ./scenario_small.json --timeline-log timeline_ark_go_small.ndjson --event-log event_ark_go_small.ndjson
./donburi_go/donburi_go --scenario ./scenario_small.json --timeline-log timeline_donburi_go_small.ndjson --event-log event_donburi_go_small.ndjson
./soa_go/soa_go --scenario ./scenario_small.json --timeline-log timeline_soa_go_small.ndjson --event-log event_soa_go_small.ndjson
./aos_go/aos_go --scenario ./scenario_small.json --timeline-log timeline_aos_go_small.ndjson --event-log event_aos_go_small.ndjson

echo "\n"
echo "Running benchmarks with middle scenario..."
./soa_rs/target/release/soa_rs --scenario ./scenario_middle.json --timeline-log timeline_soa_rs_middle.ndjson --event-log event_soa_rs_middle.ndjson
./aos_rs/target/release/aos_rs --scenario ./scenario_middle.json --timeline-log timeline_aos_rs_middle.ndjson --event-log event_aos_rs_middle.ndjson
./hecs_rs/target/release/hecs_rs --scenario ./scenario_middle.json --timeline-log timeline_hecs_rs_middle.ndjson --event-log event_hecs_rs_middle.ndjson
./bevy_rs/target/release/bevy_rs --scenario ./scenario_middle.json --timeline-log timeline_bevy_rs_middle.ndjson --event-log event_bevy_rs_middle.ndjson
./ark_go/ark_go --scenario ./scenario_middle.json --timeline-log timeline_ark_go_middle.ndjson --event-log event_ark_go_middle.ndjson
./donburi_go/donburi_go --scenario ./scenario_middle.json --timeline-log timeline_donburi_go_middle.ndjson --event-log event_donburi_go_middle.ndjson
./soa_go/soa_go --scenario ./scenario_middle.json --timeline-log timeline_soa_go_middle.ndjson --event-log event_soa_go_middle.ndjson
./aos_go/aos_go --scenario ./scenario_middle.json --timeline-log timeline_aos_go_middle.ndjson --event-log event_aos_go_middle.ndjson

echo "\n"
echo "Running benchmarks with large scenario..."
./soa_rs/target/release/soa_rs --scenario ./scenario.json --timeline-log timeline_soa_rs_large.ndjson --event-log event_soa_rs_large.ndjson
./aos_rs/target/release/aos_rs --scenario ./scenario.json --timeline-log timeline_aos_rs_large.ndjson --event-log event_aos_rs_large.ndjson
./hecs_rs/target/release/hecs_rs --scenario ./scenario.json --timeline-log timeline_hecs_rs_large.ndjson --event-log event_hecs_rs_large.ndjson
./bevy_rs/target/release/bevy_rs --scenario ./scenario.json --timeline-log timeline_bevy_rs_large.ndjson --event-log event_bevy_rs_large.ndjson
./ark_go/ark_go --scenario ./scenario.json --timeline-log timeline_ark_go_large.ndjson --event-log event_ark_go_large.ndjson
./donburi_go/donburi_go --scenario ./scenario.json --timeline-log timeline_donburi_go_large.ndjson --event-log event_donburi_go_large.ndjson
./soa_go/soa_go --scenario ./scenario.json --timeline-log timeline_soa_go_large.ndjson --event-log event_soa_go_large.ndjson
./aos_go/aos_go --scenario ./scenario.json --timeline-log timeline_aos_go_large.ndjson --event-log event_aos_go_large.ndjson
