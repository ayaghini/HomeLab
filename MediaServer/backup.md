# Backing Up Movies from OMV to TrueNAS using Rsync

I spent some time exploring options to back up my movies on the **OMV (OpenMediaVault)** system. Rsync seemed to be the way to go, but for some reason, many people online tried using the SSH method, which made the process more complicated than necessary.

After some testing, I followed this helpful video:  
🔗 [YouTube Tutorial](https://www.youtube.com/watch?v=7GFZWNfwgbA)

---

## Summary of Steps

### 1. On the OMV Server
1. **Enable the Rsync Server:**
   - Go to **Services → Rsync → Server** and enable it.
2. **Create a New Module:**
   - Under the **Modules** tab, click **Add**.
   - Select the folder you want to back up (in my case, my entire `mediaPool`).
   - Set a **name** for this module (you’ll use this name later in TrueNAS under Rsync Task configuration).
   - Leave most of the other settings unchanged; TrueNAS will handle those during the backup.
3. **Create a User (if needed):**
   - Go to **Users → Users**.
   - Create a new user (for example, `omvuser`) and grant it permission to access the folder you wish to back up.

---

### 2. On the TrueNAS Server
1. Go to **Data Protection → Rsync Tasks** and **add a new Rsync task**.
2. **Configure the Task:**
   - **Path:** Select the local directory where you want to store the backup (I created a folder in my main pool for this).
   - **Remote Host:** Use the format `omvuser@<OMV_IP>` (replace with your actual OMV username and IP address).
   - **Remote Module Name:** Enter the module name you created earlier on OMV.
   - **Direction:** Select **Pull**, since TrueNAS will be pulling data from OMV.
3. **Save and Run the Task.**

If everything is configured correctly, the Rsync process should start and your data will be backed up to TrueNAS.

---

## Notes
- In my setup, I didn’t encounter any password prompt, and the connection works perfectly fine.
- Make sure your OMV firewall or router allows Rsync traffic (default port 873).

---

✅ **Result:** Backup from OMV to TrueNAS using Rsync works reliably and without needing SSH complexity.


