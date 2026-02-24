# Networking Basics (Homelab)

This file is a quick operational checklist for the home network.

## VLAN fundamentals

- Use VLANs to isolate traffic by trust boundary, not by device brand.
- Typical split:
- `VLAN 10`: management
- `VLAN 20`: servers
- `VLAN 30`: IoT
- `VLAN 40`: clients
- `VLAN 50`: guest
- Default policy should be deny between VLANs, then allow only required flows.

## DHCP and DNS

- Keep one DHCP authority per VLAN.
- Use static DHCP reservations for infrastructure devices.
- Keep internal DNS names stable (`host.lab.local` style).
- Track every static IP in one source of truth (sheet or repo doc).

## Firewall policy checklist

- Allow client VLAN to internet.
- Block guest VLAN to internal subnets.
- Allow Home Assistant to IoT only on required ports.
- Allow management VLAN to all infra devices.
- Log denied inter-VLAN traffic during initial rollout.

## Operational checks

- Confirm switch trunk ports carry only needed VLANs.
- Confirm access ports are untagged on exactly one VLAN.
- Test from each VLAN:
- DNS resolve
- gateway reachability
- internet access
- blocked east-west traffic

## Reference

- VLAN explainer (Robotics Overlords): https://www.youtube.com/watch?v=XdqP14NclZ0&t=978s
