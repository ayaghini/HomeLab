# Automated Ripping Machine (ARM) Setup Guide

## Overview
I am going to create an Automated Ripping Machine (ARM). I will be following this guide: [YouTube Guide](https://www.youtube.com/watch?v=wPWx6GISIhY&t=706s). end up using this: [GitHub Page](https://github.com/automatic-ripping-machine/automatic-ripping-machine/discussions/965) 

## Quick Note (upgrading)
see: [Updating Docker](https://github.com/automatic-ripping-machine/automatic-ripping-machine/wiki/Docker-Upgrading)

docker stop arm-rippers
docker pull automaticrippingmachine/automatic-ripping-machine
docker container prune
cd /home/arm/
./start_arm_container.sh

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

# Bonus: Supporting Intel Quick Sync for ARM (Automatic Ripping Machine)

## ⚠️ Problem

I was getting **13–18 fps** for decoding, which is quite low.  
To improve performance, I decided to enable **Intel Quick Sync** support.

Reference:  
🔗 [GitHub Issue & Solution](https://github.com/automatic-ripping-machine/automatic-ripping-machine/issues/1248#issuecomment-2520524653)

---

## 🛠 Steps to Enable Quick Sync

### 1. Create a Custom Dockerfile

Inside the `arm` folder:

```bash
nano Dockerfile
```

Paste the following content:

```Dockerfile
FROM automaticrippingmachine/automatic-ripping-machine:latest 
LABEL desc="ARM w/ Intel Quick Sync Video Support"

RUN apt update -y && \
    apt upgrade -y && \
    apt install -y libmfx1 libmfx-tools libmfx-dev vorbis-tools wget && \
    mkdir -p quicksync && cd quicksync && \
    git clone https://github.com/intel/libva.git libva && \
    cd libva && \
    ./autogen.sh && \
    make && make install && \
    cd .. && \
    wget https://github.com/Intel-Media-SDK/MediaSDK/releases/download/intel-mediasdk-21.3.5/MediaStack.tar.gz && \
    tar -xvf MediaStack.tar.gz && \
    cd MediaStack && \
    bash install_media.sh && \
    cd .. && \
    git clone https://github.com/Intel-Media-SDK/MediaSDK msdk && \
    cd msdk && \
    mkdir build && cd build && \
    cmake .. && \
    make && make install && \
    cd ../.. && \
    rm -rf quicksync
```

Then build the Docker image:

```bash
docker build -t arm-qsv .
```

⏳ **Note:** This will take a while.

After completion:

```bash
docker images
```

You should see `arm-qsv`.

---

### 2. Create `start_arm_container.sh`

```bash
nano start_arm_container.sh
```

Update the image name and add the rendering device:

```bash
#!/bin/bash
docker run -d \
    -p "8080:8080" \
    -e ARM_UID="1000" \
    -e ARM_GID="1000" \
    -v "/home/arm:/home/arm" \
    -v "/home/arm/music:/home/arm/music" \
    -v "/home/arm/logs:/home/arm/logs" \
    -v "/home/arm/media:/home/arm/media" \
    -v "/home/arm/config:/etc/arm/config" \
    --device="/dev/sr0:/dev/sr0" \
    --device="/dev/sg0:/dev/sg0" \
    --device="/dev/dri/renderD128:/dev/dri/renderD128" \
    --privileged \
    --restart "always" \
    --name "arm-rippers" \
    --cpuset-cpus='0-5' \
    arm-qsv:latest
```

Then run:

```bash
docker stop arm-rippers
docker rm arm-rippers
./start_arm_container.sh
```

---

### 3. Update `config/arm.yaml`

```bash
nano config/arm.yaml
```

Add or update the following:

```yaml
HB_PRESET_DVD: "H.265 QSV 1080p"
HB_PRESET_BD: "H.265 QSV 2160p 4K"

# Additional HandBrake arguments for DVDs.
HB_ARGS_DVD: "--subtitle scan -F --quality=18 --encoder-preset=quality"

# Additional Handbrake arguments for Bluray Discs.
HB_ARGS_BD: "--subtitle scan -F --subtitle-burned --audio-lang-list eng --all-audio --quality=22 --encoder-preset=quality"
```

Save and exit, then restart the container:

```bash
docker restart arm-rippers
```

✅ You should now have hardware-accelerated encoding using Intel Quick Sync.

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

### Transferring Files to NAS Using `rsync` and CIFS Mount

Here's a quick guide to mount your NAS and transfer files to it using `rsync`.

---

#### 📁 Step 1: Create a Mount Point

```bash
mkdir /media/mediaNASmnt
```

---

#### 🔗 Step 2: Mount the NAS Share

```bash
sudo mount -t cifs -o username=yaghini,password=mypass,uid=1000,gid=1000,dir_mode=0775,file_mode=0664 //192.168.3.225/mediaNAS /media/mediaNASmnt
```

Replace:
- `yaghini` with your NAS username  
- `mypass` with your NAS password  
- `192.168.3.225` with your NAS IP  
- `mediaNAS` with your actual share name

---

#### 🔄 Step 3: Transfer Files with `rsync`

```bash
rsync -a /home/arm/media/completed/ /media/mediaNASmnt/
```

This will sync all files from your source folder to the mounted NAS directory.

---

📝 **Note**: The `-a` flag in `rsync` preserves permissions, symbolic links, and timestamps.




