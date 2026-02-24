# Raspberry Pi Server

This folder tracks standalone Raspberry Pi server services that are not part of the cluster.

## Planned services

- Pi-hole or AdGuard Home
- VPN endpoint (WireGuard)
- Utility services (uptime monitor, small automations)

## Baseline setup checklist

1. Install Raspberry Pi OS Lite 64-bit.
2. Enable SSH and set static DHCP reservation.
3. Apply updates: `sudo apt update && sudo apt full-upgrade -y`.
4. Configure hostname and timezone.
5. Install baseline packages: `git`, `curl`, `vim`, `htop`, `ufw`.
6. Add SSH public key and disable password auth when ready.

## Security baseline

- Change default password immediately.
- Use key-based SSH auth.
- Restrict inbound ports with firewall rules.
- Keep unattended upgrades enabled for security patches.

## TODO

- Add per-service installation notes.
- Add backup and restore procedure.
- Add monitoring and alerting checks.
