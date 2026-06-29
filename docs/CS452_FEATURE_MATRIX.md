# CS452 ROTOS Feature Matrix

**Canonical references:**
- **SMP-line:** [CS452ROTOS-SMP-DarcyOS](https://github.com/codejedi-ai/CS452ROTOS-SMP-DarcyOS)
- **APU-line:** [CS452ROTOS-APU-SmpOS](https://github.com/codejedi-ai/CS452ROTOS-APU-SmpOS) (**SmpOS** — canonical NIX APU stack; SMP multikernel research home)

Family-wide copy lives in each kernel repo under `docs/CS452_FEATURE_MATRIX.md`.

## Legend

| Symbol | Meaning |
|--------|---------|
| ● | Implemented and in use |
| ◐ | Partial / design only / scaffold |
| ○ | Not present |
| — | N/A (infra repo) |

### Multi-core models (do not conflate)

| Model | Description | Repos |
|-------|-------------|-------|
| **SMP multikernel** | 4 independent kernels (one per core); local `Send`/`Receive`; cross-core **`CoreSend`** + shared mailbox + **IPI**; LNS / CNS / Core Name Notifier | **SmpOS** (design target) |
| **APU / accel** | Core 0 = full OS scheduler; cores 1–3 = bare-metal **job workers** (`accel_dispatch`, APUServer) — not full kernels | **SmpOS** (canonical); KatarOS, NyxOS, AtariOS (variants) |
| **Single-core** | One scheduler, one ready queue (classic CS452) | DarcyOS, IrisOS, MekkanaOS, PrimeOS, TRAINS |

---

## Master table

| Repo | OS | Family | K depth | IRQ UART | Shell | TC1 trains | APU workers | SMP multikernel | Docker platform | Status |
|------|-----|--------|---------|----------|-------|------------|-------------|-----------------|-----------------|--------|
| `CS452ROTOS-SMP-DarcyOS` | DarcyOS | SMP-line | k0–k4 + tc1 | ● | `commands_shell` | ● | ○ | ○ | ● `./dev.sh make all` | **Canonical product** |
| `CS452ROTOS-SMP-IrisOS` | IrisOS | SMP-line | k0–k4 + tc1 | ● | same | ● | ○ | ○ | ● | DarcyOS fork |
| `CS452ROTOS-SMP-MekkanaOS` | MekkanaOS | SMP-line | k0–k4 + tc1 | ● | after K-tests | ● | ○ | ○ | ● `MODE=qemu` | Refactor + boot tests |
| `CS452ROTOS-SMP-PrimeOS` | PrimeOS | SMP-line | k0–k4 + tc1 | ● | `commands_shell` | ● | ○ | ○ | ● | GitHub mirror |
| `uwaterloo_…-cs452-trains` | PrimeOS | SMP-line | k0–k4 + tc1 | ● | `commands_shell` | ● | ○ | ○ | ● | Course hand-in (GitLab) |
| `CS452ROTOS-APU-SmpOS` | SmpOS | APU-line | layer0–5 | ● | display shell | ● | ● APUServer | ◐ **SMP lead** | ● `./dev.sh make all` | **Canonical APU** |
| `CS452ROTOS-APU-KatarOS` | KatarOS | APU-line | layer0–5 | ● | display shell | ● | ● APUServer | ○ | ● `./dev.sh make all` | APU variant (production NIX) |
| `CS452ROTOS-APU-NyxOS` | NyxOS | APU-line | layer0–5 | ● | display shell | ● | ● APUServer | ○ | ● `./dev.sh make all` | APU variant (Docker-pinned) |
| `CS452ROTOS-APU-AtariOS` | AtariOS | APU-line | layer0–5 | ● | display + `ConsoleGetc` | ◐ | ● APUServer | ○ | ● | APU variant (NIX snapshot) |
| `CS452ROTOS-PLATFORM` | — | — | — | — | — | — | — | — | ● `./build.sh` | Shared toolchain / QEMU image |

---

## SMP focus (single-core kernels only)

**Active SMP program:** **`CS452ROTOS-APU-SmpOS`** (canonical APU + SMP lead), then **DarcyOS**, then other SMP-line mirrors.

**Deferred:** KatarOS, NyxOS, AtariOS — track **canonical APU (SmpOS)** for stack changes; SMP multikernel port to those variants is Phase B.

| SMP multikernel focus | Single-core (SMP target) | APU variants (track SmpOS) |
|----------------------|--------------------------|----------------------------|
| **SmpOS** ◐ design — **canonical APU + SMP lead** | DarcyOS, IrisOS, MekkanaOS, PrimeOS, TRAINS ○ | KatarOS, NyxOS, AtariOS ● workers |

---

## Unified I/O stack (interrupt-driven console)

Active train kernels (SMP-line and NIX APU-line):

```
gic_init() → UART IRQ 153 → io_notifier → UART1_CONSOLE_server → ConsoleGetc
```

| Line | UART path |
|------|-----------|
| **SMP-line** | `src/k4/servers/{io_notifier,UART1_CONSOLE_server,UART2_MARKLIN_server,io_api}/` |
| **NIX** | `src/layer3-services/uart/` |

---

## Unified Docker build

| Item | Value |
|------|-------|
| Image | `codejedi-ai/cs452rotos-platform:latest` |
| Commands | `./dev.sh build-image` · `./dev.sh make all` · `./dev.sh run` · `./dev.sh shell` |

---

## Docs (per repo under `docs/`)

| Doc | Purpose |
|-----|---------|
| [CS452_FEATURE_MATRIX.md](CS452_FEATURE_MATRIX.md) | This table |
| [CODE_EVOLUTION_TREE.md](docs/CODE_EVOLUTION_TREE.md) | Feature evolution (not git) |
| [APU_MULTI_CORE.md](docs/APU_MULTI_CORE.md) | APU worker model |
| [SMP_MULTIKERNEL_IPC.md](docs/SMP_MULTIKERNEL_IPC.md) | SMP target architecture (`CoreSend`, IPI, LNS/CNS) |
| [SMP_ROADMAP_ALL_KERNELS.md](docs/SMP_ROADMAP_ALL_KERNELS.md) | **Per-kernel SMP rollout plans** |

---

## SMP multikernel (in progress on canonical APU)

- **SmpOS** (`CS452ROTOS-APU-SmpOS`) — **canonical APU-line kernel** and **primary SMP implementation target** (`CoreSend`/IPI on top of today's APUServer model). See [SMP_ROADMAP_ALL_KERNELS.md](docs/SMP_ROADMAP_ALL_KERNELS.md) Phase A0.
- **KatarOS, NyxOS, AtariOS** — APU-line variants; inherit stack from SmpOS; SMP multikernel deferred until proven on SmpOS + DarcyOS.
