# Proxmox Notes

This folder stores practical notes for the Proxmox layer in this homelab.

## Current docs

- Proxmox Backup Server setup: `proxmox_backup_server.md`

## PCIe passthrough baseline

- Keep host and guest ownership separate.
- Do not configure or attach a PCIe device in the Proxmox host if you plan to pass it through to a VM.
- Confirm IOMMU is enabled in BIOS and kernel config before passthrough testing.
- Validate IOMMU groups first, then assign full device functions to a single VM.
- After passthrough, verify device health inside the guest, not on the host.

Reference walkthrough:
- https://www.youtube.com/watch?v=_hOBAGKLQkI

## TODO

- Add a documented baseline Proxmox host install checklist.
- Add VM template notes (Debian, Ubuntu, cloud-init defaults).
- Add standard backup job policy linked to PBS datastore naming.
