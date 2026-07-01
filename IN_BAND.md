# In-band task quality notes

Knotwork is shaped for structured parser and lifecycle analysis, not one-field
crashes. Good findings should require a valid container, valid earlier records,
state replacement or replay, and a later operation that observes the composed
state.

Preferred task-description shape:

```text
A <bug-class> occurs in <exact::symbol> when a <valid envelope/package/scene>
loads prior records, replaces or replays related state, then performs a later
<inspection/resolve/commit> operation that observes the stale or mismatched
state.
```

Do not submit descriptions for trivial size/count checks. Do not describe the
fix. Do not claim a memory-safety issue without a sanitizer-confirmed crash.

Current hardened paths:

- `LLVMFuzzerTestOneInput` runs scene load -> flatten -> bounds.
- Mesh bytes run binary mesh load -> BVH build -> raycast.
- Package bytes run package load -> project reconstruction -> inspection.
- Editor replay bytes run project setup -> selection -> transform command ->
  undo/redo -> structured patch -> scene replacement -> undo/redo -> inspect.
- Import bytes run scene import, then package import fallback.

This gives fuzzing a multi-step sequence resembling real editor work while
keeping object ownership explicit and safe.
