# 🏡 HomeLab

Welcome to my **experimental homelab journey** — a living notebook where I test ideas, break things on purpose, and document everything so you (and future‑me) can rebuild it from scratch.

> **Goal:** Run production‑style services on consumer‑grade hardware while honing skills in Linux, networking, automation, and self‑hosting.

---

## 📚 Repository map

| Folder | What you’ll find | Highlights |
|--------|------------------|------------|
| `Hardware/` | Bill of materials, rack layout & power budget | Low‑power mini‑PC cluster • 10 GbE backbone • UPS & PDU notes |
| `ProxMox/` | IaC for the hypervisor layer | Terraform • cloud‑init • HA cluster config |
| `TrueNas/` | ZFS storage design & backup strategy | Pool layout • snapshot & replication jobs |
| `Networking/` | RouterOS / pfSense snippets, VLAN plan, DNS & DHCP | WireGuard site‑to‑site • mDNS reflector |
| `Home Assistant/` | Smart‑home automations & dashboards | Zigbee2MQTT • ESPHome • presence detection |
| `MediaServer/` | Plex / Jellyfin / *arr stack compose files | GPU transcoding • automated media acquisition |
| `NextCloud/` | Private cloud & file‑sync | Ansible role • S3‑backed external storage |
| `LLM/` | Local‑first AI experiments | Ollama • llama.cpp with GPU acceleration |
| `Raspberry pi Cluster/` | k3s on 4 × RPi 4 | GitOps pipeline • MetalLB • sealed‑secrets |
| `Raspberry pi Server/` | Solo SBC utilities | Pi‑hole • Pi‑KVM |

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

# 2. Read hardware & network docs
less Hardware/README.md
less Networking/README.md

# 3. Bootstrap Proxmox
cd ProxMox/bootstrap
ansible-playbook site.yml --ask-become-pass
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
