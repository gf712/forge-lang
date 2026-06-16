// Roundtrip test for the forge dialect's comptime types and value attributes.
// Parses the textual IR and re-prints it twice; the second pass proves the
// printed form is itself parseable (idempotence).
// RUN: %forge-opt %s | %forge-opt | %FileCheck %s

// The module printer sorts discardable attributes by name, so the canonical
// order is f, i, tf, ti.
// CHECK:      module attributes {
// CHECK-SAME:   forge.f = #forge.comptime_float<3.500000e+00> : !forge.comptime_float
// CHECK-SAME:   forge.i = #forge.comptime_int<42> : !forge.comptime_int
// CHECK-SAME:   forge.tf = !forge.comptime_float
// CHECK-SAME:   forge.ti = !forge.comptime_int
module attributes {
  forge.i = #forge.comptime_int<42>,
  forge.f = #forge.comptime_float<3.5>,
  forge.ti = !forge.comptime_int,
  forge.tf = !forge.comptime_float
} {
}
