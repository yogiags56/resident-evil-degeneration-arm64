# Stage 124G — stdio lifecycle and seek-state consistency

## Changes
- Centralize per-slot stream state reset for readability/writability/append flags, EOF, error, and unread-buffer length.
- Reset the slot before installing a newly opened backend stream.
- Reset per-stream state after `fclose` and provider-wide close.
- Flush writable streams during provider-wide shutdown when the optional backend flush callback exists.
- Mark stream error when seek/tell operations cannot be completed by the backend or return invalid positions.
- Add regression coverage that closes a stream with unread pushback data, reopens the same slot, checks a new generation token and clean EOF/error indicators, and reads the new stream's first byte without stale buffered data.

## Validation
Host regression and Android ARM64 native build are required on the final branch head. No original game boot or gameplay is claimed.
