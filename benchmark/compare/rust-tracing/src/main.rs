use std::hint::black_box;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Instant;

use serde_json::json;
use tracing::span::{Attributes, Record};
use tracing::subscriber::with_default;
use tracing::{debug, info, span, Event, Id, Level, Metadata, Subscriber};

const LOG_ITERATIONS: u64 = 200_000;
const ENABLED_ITERATIONS: u64 = 100_000;
const SPAN_ITERATIONS: u64 = 20_000;
const OTLP_ITERATIONS: u64 = 10_000;
const OTLP_BATCH_SIZE: u64 = 8;

struct NoopSubscriber {
    enabled: bool,
    next_id: AtomicU64,
}

impl NoopSubscriber {
    fn enabled() -> Self {
        Self {
            enabled: true,
            next_id: AtomicU64::new(1),
        }
    }

    fn disabled() -> Self {
        Self {
            enabled: false,
            next_id: AtomicU64::new(1),
        }
    }
}

impl Subscriber for NoopSubscriber {
    fn enabled(&self, _metadata: &Metadata<'_>) -> bool {
        self.enabled
    }

    fn new_span(&self, _span: &Attributes<'_>) -> Id {
        Id::from_u64(self.next_id.fetch_add(1, Ordering::Relaxed))
    }

    fn record(&self, _span: &Id, _values: &Record<'_>) {}

    fn record_follows_from(&self, _span: &Id, _follows: &Id) {}

    fn event(&self, _event: &Event<'_>) {}

    fn enter(&self, _span: &Id) {}

    fn exit(&self, _span: &Id) {}
}

fn measure_disabled_event() {
    with_default(NoopSubscriber::disabled(), || {
        let start = Instant::now();
        for i in 0..LOG_ITERATIONS {
            debug!(value = black_box(i), "value");
        }
        let ns = start.elapsed().as_nanos() as f64;
        println!(
            "R1-RustTracingDisabled workload={} build=Release framework=tracing ns_per_event={}",
            LOG_ITERATIONS,
            ns / LOG_ITERATIONS as f64
        );
    });
}

fn measure_enabled_noop_event() {
    with_default(NoopSubscriber::enabled(), || {
        let start = Instant::now();
        for i in 0..ENABLED_ITERATIONS {
            info!(value = black_box(i), "value");
        }
        let ns = start.elapsed().as_nanos() as f64;
        println!(
            "R2-RustTracingEnabledNoop workload={} build=Release framework=tracing ns_per_event={}",
            ENABLED_ITERATIONS,
            ns / ENABLED_ITERATIONS as f64
        );
    });
}

fn measure_span_scope() {
    with_default(NoopSubscriber::enabled(), || {
        let start = Instant::now();
        for _ in 0..SPAN_ITERATIONS {
            let current = span!(Level::INFO, "bench");
            let guard = current.enter();
            black_box(&guard);
        }
        let ns = start.elapsed().as_nanos() as f64;
        println!(
            "R3-RustTracingSpanScopeNoop workload={} build=Release framework=tracing ns_per_scope={}",
            SPAN_ITERATIONS,
            ns / SPAN_ITERATIONS as f64
        );
    });
}

fn measure_serde_json_otlp() {
    let span_ids = [
        "00f067aa0ba902b7",
        "00f067aa0ba902b8",
        "00f067aa0ba902b9",
        "00f067aa0ba902ba",
        "00f067aa0ba902bb",
        "00f067aa0ba902bc",
        "00f067aa0ba902bd",
        "00f067aa0ba902be",
    ];
    let start = Instant::now();
    let mut bytes = 0usize;
    for _ in 0..OTLP_ITERATIONS {
        let spans = span_ids
            .iter()
            .map(|span_id| {
                json!({
                    "traceId": "4bf92f3577b34da6a3ce929d0e0e4736",
                    "spanId": span_id,
                    "name": "otlp-bench-span",
                    "kind": "SPAN_KIND_INTERNAL",
                    "traceState": "vendor=value"
                })
            })
            .collect::<Vec<_>>();
        let body = json!({
            "resourceSpans": [{
                "scopeSpans": [{
                    "scope": {"name": "galay-tracing"},
                    "spans": spans
                }]
            }]
        })
        .to_string();
        bytes += black_box(body.len());
    }
    let ns = start.elapsed().as_nanos() as f64;
    println!(
        "R4-RustSerdeJsonOtlp workload={} batch_size={} build=Release framework=serde_json ns_per_span={} avg_body_bytes={}",
        OTLP_ITERATIONS,
        OTLP_BATCH_SIZE,
        ns / (OTLP_ITERATIONS * OTLP_BATCH_SIZE) as f64,
        bytes / OTLP_ITERATIONS as usize
    );
}

fn main() {
    measure_disabled_event();
    measure_enabled_noop_event();
    measure_span_scope();
    measure_serde_json_otlp();
}
