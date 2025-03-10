# Automated Ripping Machine (ARM) Setup Guide

## Overview
I am going to create an Automated Ripping Machine (ARM). I will be following this guide: [YouTube Guide](https://www.youtube.com/watch?v=wPWx6GISIhY&t=706s).

## Step 1: Install Proxmox

- Install **Proxmox** on your system.
- Install **Debian 12 Linux** inside Proxmox.

## Step 2: Add User to Sudo

Run the following commands:
```sh
su -
usermod -aG sudo yourusername
```
Then restart the computer.

## Step 3: Enable Clipboard for VNC
Following this guide: [Proxmox Forum Thread](https://forum.proxmox.com/threads/tl-dr-for-getting-novnc-copy-paste-clipboard-sharing-working-with-ubuntu-24-guest.159051/)

1. Open Proxmox UI
2. Navigate to **Hardware → Display → Advanced**
3. Set **Clipboard** to `VNC`

## Step 4: Add a Serial Port to Your VM

1. Add a serial port to your VM.
2. Configure the serial port in your GRUB configuration:
   ```sh
   sudo nano /etc/default/grub
   ```
3. Modify the following line:
   ```sh
   GRUB_CMDLINE_LINUX_DEFAULT="console=ttyS0,115200"
   ```
4. Update GRUB:
   ```sh
   sudo update-grub
   ```
5. Reboot the system.

After reboot, the serial terminal with copy/paste should work.

## Step 5: Pass Through the Blu-ray Drive to VM

1. Identify the Blu-ray drive:
   ```sh
   lsblk -f
   ```
   - Example output: `sr0`
2. Edit Proxmox VM configuration:
   ```sh
   sudo nano /etc/pve/qemu-server/<vmid>.conf
   ```
3. Add the following line:
   ```sh
   scsi1: /dev/sr0,media=cdrom
   ```

## Step 6: Install ARM Using Docker

Follow this guide: [ARM Docker Guide](https://github.com/automatic-ripping-machine/automatic-ripping-machine/wiki/docker)

1. Get user and group ID:
   ```sh
   id -u arm
   id -g arm
   ```
2. Add these values to `start_arm_container.sh` under `/home/arm`.
3. Create the required directories as mentioned in the `start_arm_container.sh` file:
   ```sh
   mkdir -p /home/arm/{raw,completed,logs,config}
   ```
4. Change ownership of the directories:
   ```sh
   sudo chown -R arm:arm /home/arm/*
   ```

## Step 7: Access the ARM Web UI

Once installed, access the ARM web interface at:
```
http://192.168.3.32:8080/
```

## Step 8: Get OMDB API Key

1. Obtain an OMDB API key from [OMDB API](https://www.omdbapi.com/apikey.aspx).
2. Open the ARM configuration file:
   ```sh
   sudo nano ./config/arm.yaml
   ```
3. Add the API key at the very end of the file.
4. Restart the ARM Docker container:
   ```sh
   docker restart arm-rippers
   ```

## Troubleshooting: Disk Not Recognized Automatically

### Problem:
The Blu-ray drive is not being passed through completely to the VM, causing automatic detection issues.

### Solution:
- The Blu-ray drive must be fully passed through via the **SATA controller**.
- If the SATA controller is shared with your hard drive, it cannot be passed through directly.
- Follow this discussion for alternatives: [ARM GitHub Discussion](https://github.com/automatic-ripping-machine/automatic-ripping-machine/discussions/965).


---

## 📂 Step 5: Share the Completed Folder with Jellyfin

1. **Install Samba**:
   ```bash
   sudo apt update
   sudo apt install samba -y
   ```

2. **Edit Samba Configuration**:
   ```bash
   sudo nano /etc/samba/smb.conf
   ```

3. **Add the Following to the Configuration File**:
   ```ini
   [Media]
   path = /home/arm/media/completed
   browseable = yes
   read only = yes
   guest ok = yes
   ```

4. **Restart Samba for Changes to Take Effect**:
   ```bash
   sudo systemctl restart smbd
   ```

---


