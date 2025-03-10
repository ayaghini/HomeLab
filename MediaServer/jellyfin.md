# 📌 Setting Up Jellyfin on Proxmox with Access to ARM Media

## 🎥 Reference Video
I am following this tutorial:  
[How to Install Jellyfin on Proxmox](https://www.youtube.com/watch?v=FtvobJjptbs&t=7s)

---

## 🖥️ Step 1: Install Ubuntu Template & Create a Container

1. **Proxmox UI Steps**:
   - Go to **Local Storage** → **Download Ubuntu Template**.
   - Create a new **CT (Container)**.
   - **Untick "Unprivileged Container"**.
   - **Do not start** the container yet.

2. **Enable Required Options**:
   - Go to **CT Options**.
   - Enable:
     - **Nesting**
     - **SMB**
     - **NFS**
     - **CIFS**

3. **Start the Container**.

---

## 🛀 Step 2: Update the Container

Once the container is running, access the console and update the system:

```bash
apt update && apt upgrade -y
```

---

## 📁 Step 3: Mount the Shared Folder from ARM

1. **Install CIFS Utilities**:
   ```bash
   sudo apt update
   sudo apt install cifs-utils -y
   ```

2. **Create a Mount Point**:
   ```bash
   sudo mkdir -p /mnt/media
   ```

3. **Mount the Shared Folder**:
   ```bash
   sudo mount -t cifs //IP_OF_ARM_CT/Media /mnt/media -o guest,uid=1000,gid=1000
   ```

4. **Verify the Mount**:
   ```bash
   ls /mnt/media
   ```
   - If the media files are visible, the mount was successful.

5. **Make the Mount Permanent**:  
   Edit `/etc/fstab` and add:
   ```
   //IP_OF_ARM_CT/Media /mnt/media cifs guest,uid=1000,gid=1000 0 0
   ```

---

## 🎮 Step 4: Install Jellyfin

Follow the **official Jellyfin installation guide**:  
🔗 [Jellyfin Installation Docs](https://jellyfin.org/docs/general/installation/linux)

1. **Install Curl**:
   ```bash
   apt install curl -y
   ```

2. **Run the Jellyfin Installer**:
   ```bash
   curl https://repo.jellyfin.org/install-debuntu.sh | sudo bash
   ```

---

## ✅ Final Steps:
- Open **Jellyfin Web UI** (`http://<CT_IP>:8096`).
- Add the **media library path**: `/mnt/media/`.
- Start enjoying your movies! 🎥🍿



