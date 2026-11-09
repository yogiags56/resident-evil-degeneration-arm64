# Stage 120S — ELF32 ARM PT_LOAD Segment Mapping & Memory Layout Planner

## Objective

Add a bounded, read-only planning layer for ELF32 EM_ARM PT_LOAD segments.
This bridges static ELF inventory and a future reviewed guest loader design
without performing memory allocation, copying, zero-fill, relocation, or code
execution.

## Implemented

- Validate ET_EXEC/ET_DYN and require at least one PT_LOAD.
- Report each segment's file-backed, memory, BSS zero-fill, and page-rounded
  virtual ranges.
- Preserve segment permissions and flag per-segment W+X mappings.
- Detect page-rounded overlaps and report combined permissions, including
  overlaps that would produce W+X pages.
- Keep ET_EXEC fixed-address and ET_DYN runtime load-bias policies explicit.
- Check ELF32 address-space overflow and page-size bounds.
- Avoid allocating per-page arrays; overlap analysis is interval-based.

## Validation

The Stage 120S test module covers BSS boundaries, ET_EXEC/ET_DYN policy,
page overlap/W+X diagnostics, address overflow, page-size validation,
non-executable entry points, unsupported ELF types, and missing PT_LOAD.

CI validation is pending. Stage 120S must not be marked complete until the
full Python suite, host C mock compilation, and native C regressions pass.

## Safety and scope

This is a read-only layout planner. It does not allocate guest memory, map
host pointers, copy file bytes, clear BSS, apply dynamic relocations, choose
a load bias, invoke imports, or execute the ELF entry point.
