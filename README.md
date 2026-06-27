# Avaturkey

High-performance **TCP game server** for Avataria-style Flash/ActionScript clients. Players connect over a custom binary protocol on port **4333**; persistent state (accounts, inventory, rooms) is stored in **Redis**.

This is not a Redis server — Redis is only the persistence backend.

## Features

- Custom binary protocol (VLQ strings, typed values, CRC32 frames)
- Flash cross-domain policy handshake
- Session auth via Redis `auth:{key}` → `uid`
- Modular command routing (`h.*`, `o.*`, `cp.*`, …)
- Static linking (no external DLLs on Windows)

## Requirements

| Component | Version |
|-----------|---------|
| CMake | 3.16+ |
| C compiler | MSVC 2019+, GCC 11+, or Clang 14+ |
| Redis | 6+ (running locally or remotely) |

Redis must be reachable at `127.0.0.1:6379` by default. Change `REDIS_HOST` / `REDIS_PORT` in `include/const.h` if needed.

## Build

### Windows

```powershell
cmake -B build "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Binary: `build/Release/avaturkey.exe`

### Linux

```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Binary: `build/avaturkey`

## Run

1. Start Redis:

   ```bash
   redis-server
   ```

2. Start the game server:

   ```bash
   # Windows
   .\build\Release\avaturkey.exe

   # Linux
   ./build/avaturkey
   ```

Expected output:

```
Avaturkey game server ready
  TCP clients: 0.0.0.0:4333
  Redis store: 127.0.0.1:6379
```

Point your Flash client at `localhost:4333`.

## Releases

Every push to **`main`** builds Windows and Linux binaries and publishes a [GitHub Release](https://github.com/YOUR_ORG/avaturkey-async/releases) with downloadable assets (no manual tags required).

You can also run the workflow manually: **Actions → Release → Run workflow**.

Each release is named `Release #<run> (<commit>)` and includes:

- `avaturkey-windows-x64.zip`
- `avaturkey-linux-x64.zip`

## Project layout

```
├── src/              Core server, protocol, Redis client
├── include/          Public headers
├── modules/          Game command handlers (C)
├── config_all_ru/    Game XML config (clothes, maps, quests, …)
├── box/              Gift-box loot tables (JSON)
└── .github/workflows/release.yml
```

## Protocol overview

```
Client                         Server
  │                              │
  │── TCP connect :4333 ────────►│
  │◄── cross-domain policy ──────│
  │── auth frame (type 1) ──────►│  Redis: auth:{key}
  │◄── uid + session (type 1) ──│
  │── game commands (type 34) ──►│  module dispatch
  │◄── responses (type 34) ─────│
```

Command format: `[seq, "prefix.sub.cmd", { payload }]`

## License

See repository license. Game assets and config XML originate from the Avataria ecosystem.
