# 🏡 HomeLab

Welcome to my **experimental homelab journey** — a living notebook where I test ideas, break things on purpose, and document everything so you (and future‑me) can rebuild it from scratch.

> **Goal:** Run production‑style services on consumer‑grade hardware while honing skills in Linux, networking, automation, and self‑hosting.

---

## 📚 Repository map

| Folder | What you’ll find | Start here |
|--------|------------------|------------|
| `EnviroSense IoT Family/` | Air‑quality sensor projects (Pico, Touch, Ikea integration) | `EnviroSense IoT Family/EnviroSense Pico/readme.md` |
| `Hardware/` | Rackmount + wall‑mount CAD and prints | `Hardware/Rackmount/readme.md` |
| `ProxMox/` | Proxmox notes and PBS setup | `ProxMox/readme.md` |
| `TrueNas/` | TrueNAS setup notes and ZFS practices | `TrueNas/readme.md` |
| `Networking/` | Networking basics and reference links | `Networking/Basic Knowledge/basics.md` |
| `Home Assistant/` | Home Assistant integrations (Bluetti UPS notes) | `Home Assistant/readme.md` |
| `MediaServer/` | Automated ripping machine + Jellyfin/NAS notes | `MediaServer/readme.md` |
| `NextCloud/` | Nextcloud on TrueNAS (datasets + install) | `NextCloud/readme.md` |
| `LLM/` | Local LLM tooling notes | `LLM/getting_started.md` |
| `Raspberry pi Cluster/` | RPi cluster notes | `Raspberry pi Cluster/readme.md` |
| `Raspberry pi Server/` | Solo SBC utilities (placeholder) | `Raspberry pi Server/readme.md` |
| `Kids Audio Player/` | DIY Yoto‑style player build log | `Kids Audio Player/readme.md` |
| `LED Matrix/` | LED matrix CAD files | `LED Matrix/` |
| `Misc/` | One‑off CAD projects | `Misc/` |

*(If a folder is empty it’s because I haven’t finished writing things up yet — stay tuned!)*

---

## ✨ Why this repo exists

* **Learning** – Kubernetes, GitOps, ZFS, networking, monitoring, and more … all in a safe sandbox.  
* **Resilience** – Snapshots, off‑site replication, disaster‑recovery drills.  
* **Privacy‑first** – I self‑host whenever practical; cloud only when it makes sense.

---

## 🚀 Quick start

```bash
# 1. Clone the repo
git clone https://github.com/ayaghini/HomeLab.git
cd HomeLab

# 2. Start with hardware and networking notes
less "Hardware/Rackmount/readme.md"
less "Networking/Basic Knowledge/basics.md"

# 3. Core services
less "ProxMox/proxmox_backup_server.md"
less "TrueNas/readme.md"
less "Home Assistant/readme.md"
```

Then open the GitOps pipeline and start layering on services from the topic folders.

---

## 🛣️ Roadmap

- [ ] Replace Proxmox SDN with Cilium overlay.  
- [ ] Immutable OS nodes (Talos/NixOS) for the Pi cluster.  
- [ ] Off‑grid power & environmental monitoring.

---

## 🤝 Contributing

PRs that fix typos, add clarifications, or propose better ways are welcome.  
Open an issue if you hit a snag — this is a hobby project, but I try to help when I can.

---

> *“Home labs are where production nightmares are born — better here than on Friday at 5 pm.”*

© Ali Yaghini • MIT License
