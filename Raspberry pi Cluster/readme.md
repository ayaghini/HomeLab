# Raspberry Pi Cluster

## Current hardware

- 3 x Raspberry Pi 4 (8 GB)
- Rackmount design available under `Hardware/`
- Powered by PoE switch and router/switch stack

## Current status

- Raspberry Pi OS 64-bit installed
- SSH enabled
- Initial `raspi-config` run completed

## Bootstrap baseline

1. Set hostname per node (`rpi-node-1`, `rpi-node-2`, `rpi-node-3`).
2. Create static DHCP reservations in router.
3. Apply OS updates on all nodes.
4. Enable time sync and verify `timedatectl`.
5. Install common tools (`git`, `curl`, `htop`, `vim`).
6. Configure passwordless SSH admin key.

## Cluster direction

- Previous reference: Docker Swarm
- Current recommendation: k3s for better long-term ecosystem compatibility and simpler GitOps integration

Reference:
- Docker Swarm guide used initially: https://www.kevsrobots.com/blog/docker-swarm.html

## Next actions

- Decide orchestration target (Swarm or k3s) and document final choice.
- Add a one-command bootstrap script for new nodes.
- Add network and storage assumptions for workloads.

