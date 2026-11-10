# Stage 122A — Mapped multi-region ARM/Thumb CPU execution

## Goal
Connect the existing ARM/Thumb instruction decoder to the permission-checked guest memory bus so it can execute directly from separately mapped ELF PT_LOAD regions instead of requiring one contiguous legacy memory view.

## Implemented
- Added `re_guest_cpu_step_mapped` and `re_guest_cpu_run_mapped` entry points.
- Preserved the legacy `re_guest_cpu_step` and contiguous-memory execution path.
- Routed A32 instruction fetches through EXEC-checked 32-bit bus reads.
- Routed Thumb and Thumb-2 instruction fetches, including second halfwords and IT-block preflight, through EXEC-checked bus reads.
- Routed byte/halfword/word data reads through READ permission and stores through WRITE permission.
- Maps guest bus permission faults to `RE_CPU_E_PERMISSION`; missing regions remain guest memory faults and alignment faults remain distinct.
- Added a multi-region regression fixture: RX code executes MOV/LDR/STR against a separate RW data mapping; execute-from-data and write-to-code are rejected.
- Kept the CPU decoder's existing instruction semantics and legacy entry points unchanged.

## Deliberate limits
This is a CPU mapped-memory execution entry point, not yet the full runtime/ELF-loader integration. The current import dispatcher and parts of the runtime still accept the legacy contiguous memory view. Multi-region CPU execution has been verified with synthetic A32 code; Thumb/Thumb-2 mapped permission cases remain covered by follow-up tests. No claim is made that the original game boots.

## Validation
Host regression and Android ARM64 native build/ELF verification are triggered for this stage. Do not mark the stage complete until both pipelines pass.
