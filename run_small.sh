#!/bin/bash
echo "Running benchmarks with small scenario..."
./soa_rs/target/release/soa_rs --scenario ./scenario_small.json --timeline-log timeline_soa_rs.ndjson --event-log event_soa_rs_small.ndjson
./aos_rs/target/release/aos_rs --scenario ./scenario_small.json --timeline-log timeline_aos_rs_small.ndjson --event-log event_aos_rs_small.ndjson
./hecs_rs/target/release/hecs_rs --scenario ./scenario_small.json --timeline-log timeline_hecs_rs_small.ndjson --event-log event_hecs_rs_small.ndjson
./bevy_rs/target/release/bevy_rs --scenario ./scenario_small.json --timeline-log timeline_bevy_rs_small.ndjson --event-log event_bevy_rs_small.ndjson
./ark_go/ark_go --scenario ./scenario_small.json --timeline-log timeline_ark_go_small.ndjson --event-log event_ark_go_small.ndjson
./donburi_go/donburi_go --scenario ./scenario_small.json --timeline-log timeline_donburi_go_small.ndjson --event-log event_donburi_go_small.ndjson
./soa_go/soa_go --scenario ./scenario_small.json --timeline-log timeline_soa_go_small.ndjson --event-log event_soa_go_small.ndjson
./aos_go/aos_go --scenario ./scenario_small.json --timeline-log timeline_aos_go_small.ndjson --event-log event_aos_go_small.ndjson
