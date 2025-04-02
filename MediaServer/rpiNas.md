# Penta SATA HAT Setup with Raspberry Pi 5 and OpenMediaVault

## 1. Hardware Setup

Following the official Radxa guide:  
🔗 [Penta SATA HAT for Raspberry Pi 5](https://docs.radxa.com/en/accessories/penta-sata-hat/penta-for-rpi5)

- Installed the Penta SATA HAT on the Raspberry Pi 5.
- Connected all four SATA drives.
- Powered everything up.

## ❗ Issue Encountered

Only **two out of four** SATA slots were detecting hard drives.  
This may be due to power, cable, or compatibility issues with the HAT.

---

## 2. Installing OpenMediaVault (OMV)

Followed this guide:  
🔗 [Install OpenMediaVault on Raspberry Pi](https://pimylifeup.com/raspberry-pi-openmediavault/)

Steps included:

- Flashing Raspberry Pi OS Lite onto SD card.
- Updating the system packages.
- Running the OMV installation script provided in the guide.
- Accessing the OMV web UI through the Pi's IP address.

---

## Summary

- Penta SATA HAT partially functional (only 2 drives detected).
- OMV installation successful.
- Further troubleshooting needed for full HAT functionality.
