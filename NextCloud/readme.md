# Nextcloud Dataset Setup and Installation Guide

## 1. YouTube Video Reference
I am following this video tutorial:  
https://www.youtube.com/watch?v=1rpeKWGoMRY

---

## 2. Create Datasets in TrueNAS

1. **Create the `Applications` dataset**  
   - Dataset Name: `Applications`  
   - Type: **Apps**

2. **Create the `nextcloud` dataset**  
   - Parent: `Applications`  
   - Dataset Name: `nextcloud`  
   - Type: **Apps**

3. **Create child datasets under `nextcloud`**  
   | Dataset Name | Type     |
   | ------------ | -------- |
   | AppData      | Apps     |
   | UserData     | Apps     |
   | Postgres     | Generic  |

---

## 3. Set Permissions

1. Select the **parent** dataset (`Applications`) in the GUI.
2. Change **Owner** to `www-data` and **Owner Group** to `www-data`.
3. Set both **Owner** and **Group** permissions to **Full Control**.
4. **Apply recursively** to all child datasets.

### Shell Commands
```bash
sudo su
chown -R www-data:www-data /mnt/pool4tb/*
```

---

## 4. Install NextCloud

1. Go to **Apps** in the TrueNAS GUI.
2. Install **NextCloud** from the catalog.

---

## 5. Configure NextCloud

During installation, update these settings:

- **Admin Username & Password**: Choose your admin credentials.
- **Apps to Install**: Select desired NextCloud apps (e.g., Files, Calendar, etc.).
- **Host / IP Address**: Enter the IP of your TrueNAS system.
- **RADIUS Password** (if using external authentication): Set your RADIUS password.
- **Data Locations**:  
  - **AppData** dataset path  
  - **UserData** dataset path  
  - **Postgres** dataset path  
  - Ensure **automatic permissions** is **enabled** for the Postgres dataset.

---

## 6. Resolve “Not in Trusted Domain” Error

If you see the **“Not in trusted domain”** error in NextCloud:

```bash
sudo su
cd /mnt/pool4tb/Applications/appdata/config/
nano config.php
```

In `config.php`, find the `trusted_domains` array and add your machine’s IP address as a new entry:

```php
'trusted_domains' =>
  array (
    0 => 'localhost',
    1 => 'your.server.ip.address',
    // add more as needed
  ),
```

Save and exit (`Ctrl+O`, `Enter`, `Ctrl+X`), then reload NextCloud.

---

*Guide created on May 11, 2025*
