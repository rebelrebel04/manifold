from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

import yaml


@dataclass
class CoreSpec:
    id: str
    type: str          # lorenz | duffing | logistic
    params: dict = field(default_factory=dict)
    macros: dict = field(default_factory=dict)   # intensity | speed | warmth, each in [0, 1]


@dataclass
class EffectSpec:
    id: str
    type: str          # wavefolder | svf | svf_morph | granular
    params: dict = field(default_factory=dict)


@dataclass
class ModSpec:
    src: str           # "core_id.axis"  (axes: lorenz={x,y,z}, duffing={x,v}, logistic={x})
    dst: str           # "effect_id.param"
    depth: float
    scale: str = "linear"    # linear | exp2
    range: str = "bipolar"   # bipolar | positive


@dataclass
class Preset:
    name: str
    description: str = ""
    tier: str = "per_engine"           # per_engine | combo
    cores: list[CoreSpec] = field(default_factory=list)
    chain: list[EffectSpec] = field(default_factory=list)
    modulation: list[ModSpec] = field(default_factory=list)
    output_gain_db: float = 0.0
    seed: int = 0

    @classmethod
    def from_dict(cls, d: dict) -> "Preset":
        cores = [CoreSpec(id=c["id"], type=c["type"],
                          params={k: v for k, v in c.items() if k not in ("id", "type", "macros")},
                          macros=dict(c.get("macros", {})))
                 for c in d.get("cores", [])]
        chain = [EffectSpec(id=e["id"], type=e["type"],
                            params={k: v for k, v in e.items() if k not in ("id", "type")})
                 for e in d.get("chain", [])]
        mods = [ModSpec(**m) for m in d.get("modulation", [])]
        return cls(
            name=d["name"],
            description=d.get("description", ""),
            tier=d.get("tier", "per_engine"),
            cores=cores,
            chain=chain,
            modulation=mods,
            output_gain_db=float(d.get("output_gain_db", 0.0)),
            seed=int(d.get("seed", 0)),
        )

    @classmethod
    def from_yaml(cls, path: Path) -> "Preset":
        with open(path) as f:
            return cls.from_dict(yaml.safe_load(f))


def load_presets(preset_dir: Path) -> dict[str, Preset]:
    presets = {}
    for path in sorted(preset_dir.glob("*.yaml")):
        p = Preset.from_yaml(path)
        presets[p.name] = p
    return presets


def apply_overrides(preset: Preset, overrides: list[str]) -> Preset:
    # Each override: "path.to.field=value". Value parsed as YAML scalar (so numbers stay numeric).
    # Paths: "cores.<id>.<param>" | "chain.<id>.<param>" | "modulation[<i>].<field>" | "output_gain_db" | "seed"
    for ov in overrides:
        if "=" not in ov:
            raise ValueError(f"override must be key=value: {ov!r}")
        key, raw_val = ov.split("=", 1)
        val = yaml.safe_load(raw_val)
        _apply_one(preset, key.strip(), val)
    return preset


def _apply_one(preset: Preset, key: str, val):
    parts = key.split(".")
    head = parts[0]

    if head == "cores" and len(parts) == 3:
        _, cid, pname = parts
        core = next((c for c in preset.cores if c.id == cid), None)
        if core is None:
            raise KeyError(f"no core with id {cid!r}")
        core.params[pname] = val
        return

    if head == "cores" and len(parts) == 4 and parts[2] == "macros":
        _, cid, _, mname = parts
        core = next((c for c in preset.cores if c.id == cid), None)
        if core is None:
            raise KeyError(f"no core with id {cid!r}")
        core.macros[mname] = float(val)
        return

    if head == "chain" and len(parts) == 3:
        _, eid, pname = parts
        eff = next((e for e in preset.chain if e.id == eid), None)
        if eff is None:
            raise KeyError(f"no effect with id {eid!r}")
        eff.params[pname] = val
        return

    if head.startswith("modulation[") and head.endswith("]"):
        idx = int(head[len("modulation["):-1])
        fname = parts[1]
        setattr(preset.modulation[idx], fname, val)
        return

    if head == "output_gain_db":
        preset.output_gain_db = float(val)
        return
    if head == "seed":
        preset.seed = int(val)
        return

    raise KeyError(f"cannot resolve override path: {key!r}")
