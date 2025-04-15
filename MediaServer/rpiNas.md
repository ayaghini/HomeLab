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

### Setting Up ZFS on OpenMediaVault (OMV)

If you're facing issues with a HAT (Hardware Attached on Top), the main problem is often **power delivery**, especially with multiple HDDs. It's also possible one of your hard drives is faulty. After troubleshooting, I was finally able to see all three drives.

---

#### 🔧 Step 1: Install ZFS Plugin in OMV

1. Go to **Settings** → **Plugins**
2. Search for `zfs`
3. Install the plugin

Once installed, the **ZFS** option will appear under the **Storage** tab.

---

#### ⚙️ Step 2: Create ZFS Pool

- I chose the **Basic** option (no redundancy) because I will maintain backups separately.
- ZFS configuration will now be available under the **Storage** section.

---

#### 🛠️ Trouble Installing ZFS?

If you encountered an error during ZFS installation, this Reddit thread was helpful:

🔗 [Reddit: ZFS issues on OMV](https://www.reddit.com/r/OpenMediaVault/comments/1jp2l39/so_is_zfs_essentially_broken_on_omv_for_now/?rdt=45482)

---

#### 🗂️ Step 3: Set Up Shared Folder and SMB

- After creating the ZFS pool:
  - Create a **Shared Folder**
  - Enable **SMB** sharing

---

#### 🔄 Step 4: Sync Your Media

You can now begin syncing your **ripped videos** to the new ZFS pool!