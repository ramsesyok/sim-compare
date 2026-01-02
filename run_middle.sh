#!/bin/bash
echo "Running benchmarks with middle scenario..."
./aos_rs/target/release/aos_rs --scenario ./scenario_middle.json --timeline-log timeline_aos_rs_middle.ndjson --event-log event_aos_rs_middle.ndjson
./soa_rs/target/release/soa_rs --scenario ./scenario_middle.json --timeline-log timeline_soa_rs_middle.ndjson --event-log event_soa_rs_middle.ndjson
./hecs_rs/target/release/hecs_rs --scenario ./scenario_middle.json --timeline-log timeline_hecs_rs_middle.ndjson --event-log event_hecs_rs_middle.ndjson
./bevy_rs/target/release/bevy_rs --scenario ./scenario_middle.json --timeline-log timeline_bevy_rs_middle.ndjson --event-log event_bevy_rs_middle.ndjson
./aos_go/aos_go --scenario ./scenario_middle.json --timeline-log timeline_aos_go_middle.ndjson --event-log event_aos_go_middle.ndjson
./soa_go/soa_go --scenario ./scenario_middle.json --timeline-log timeline_soa_go_middle.ndjson --event-log event_soa_go_middle.ndjson
./ark_go/ark_go --scenario ./scenario_middle.json --timeline-log timeline_ark_go_middle.ndjson --event-log event_ark_go_middle.ndjson
./donburi_go/donburi_go --scenario ./scenario_middle.json --timeline-log timeline_donburi_go_middle.ndjson --event-log event_donburi_go_middle.ndjson
