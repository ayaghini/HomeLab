# Installing Proxmox Backup Server

## 1. Installation Steps

- Created a bootable USB disk with Proxmox Backup Server ISO.
- Booted from the USB disk and installed the server.
- After installation, the web UI is accessible at:

```
https://<your-server-ip>:8007
```

## 2. Disable the Enterprise Repository

The default enterprise repository requires a subscription. To disable it:

1. Edit the following file:

```bash
nano /etc/apt/sources.list.d/pbs-enterprise.list
```

2. Comment out the enterprise repository line:

```bash
# deb https://enterprise.proxmox.com/debian/pbs bookworm pbs-enterprise
```

3. Add the following repositories to `/etc/apt/sources.list` or a new `.list` file under `/etc/apt/sources.list.d/`:

```bash
deb http://deb.debian.org/debian bookworm main contrib
deb http://deb.debian.org/debian bookworm-updates main contrib

# Proxmox Backup Server - No Subscription (NOT recommended for production)
deb http://download.proxmox.com/debian/pbs bookworm pbs-no-subscription

# Security updates
deb http://security.debian.org/debian-security bookworm-security main contrib
```

4. Update the package list:

```bash
apt update
```

---

## 3. Configure Datastore

- Go to **Datastore** > **Add Datastore**.
- Choose a name unique to this Proxmox Backup Server.

---

## 4. Configure Proxmox VE to Use the Backup Server

On your Proxmox VE node:

- Go to **Datacenter** > **Storage** > **Add** > **Proxmox Backup Server**.
- Enter the server details and choose the datastore you created earlier.

---

## 5. Add Backup Job

- Navigate to **Datacenter** > **Backup** > **Add**.
- Configure your backup job to use the newly added backup server and datastore.

---

## ⚠️ Issue Encountered

By default, the backup job includes **all disks**, including data volumes. In my case, this included the entire TrueNAS storage, which I didn't intend to back up.

**Recommendation:**  
Carefully select only the VMs or containers and specific disks you want to back up. Exclude large data volumes manually when configuring the backup job.
