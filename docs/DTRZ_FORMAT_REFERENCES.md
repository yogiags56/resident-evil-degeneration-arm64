# DTRZ / Derbh format references

These public references were used as format-level corroboration. The Stage 91 parser in this repository was implemented independently and validated against the locally supplied archive; no third-party parser source was copied into the project.

1. Dade Marmalade SDK documentation — Derbh archive layout, 9-byte header, filename/folder strings, 6-byte attributes, 4-byte location header, 16-byte location records, and known method tags:
   https://dade.readthedocs.io/en/latest/api/marmalade.html
2. Marmalade-Modding community format notes — Dzip header, per-file attribute records, location header and record fields:
   https://github.com/knot126/Marmalade-Modding/blob/trunk/docs/Dzip%20format.md
3. Marmalade SDK file-system tutorial — per-file compression choices used by Derbh/Dzip archives:
   https://www.drmop.com/index.php/2011/10/07/marmalade-sdk-tutorial-dealing-with-files-and-the-file-system/

## Evidence policy

- External documentation is a hypothesis source, not proof that every game build uses an identical layout.
- The exact offsets and counts in `STAGE91_REPORT.md` were checked against the local `main.dz` archive hash recorded there.
- Unknown header/flag fields remain explicitly unknown.
- A successful extraction of this one archive profile does not prove support for every DTRZ variant.
