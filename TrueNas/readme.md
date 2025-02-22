# TrueNAS Setup & Hard Drive Best Practices

If you're setting up a **TrueNAS system**, whether on bare metal or inside **Proxmox**, there are a few key things to consider to ensure reliability and optimal performance. Below are some essential steps and resources to help you along the way.

## 🔥 Hard Drive Burn-in Testing

If you want your hard drives to last **for years**, it's crucial to perform a **burn-in test** before putting them into production. Burn-in testing helps identify early drive failures and ensures that only **healthy drives** make it into your storage array.

👉 **[Hard Drive Burn-in Testing Guide](https://www.truenas.com/community/resources/hard-drive-burn-in-testing.92/)**

## 📌 Passing Through HDDs Directly to TrueNAS (Proxmox Setup)

If you've installed **TrueNAS inside Proxmox**, one of the best configurations is to **pass the hard drives directly** to TrueNAS instead of using virtual disks. This setup allows TrueNAS to manage the drives natively, providing better performance and control over **ZFS**.

Here's the method that worked for me. Since **TrueNAS is evolving**, I will update this if I come across any changes.

- 🎥 **[Step-by-step YouTube Guide](https://www.youtube.com/watch?v=MkK-9_-2oko)**
- 📖 **[Official Proxmox Guide](https://pve.proxmox.com/wiki/Passthrough_Physical_Disk_to_Virtual_Machine_(VM))**

## 🏠 Users, ACLs & App Shares in TrueNAS

Managing **users, ACLs (Access Control Lists), and app shares** can be a complex topic in TrueNAS. If you're working with **TrueNAS SCALE 24**, I highly recommend checking out **Lawrence Systems' video**, which covers best practices and configurations.

🎥 **[TrueNAS SCALE 24 - Users & ACL Guide](https://www.youtube.com/watch?v=59NGNZ0kO04)**

## 💡 Good to Know

TrueNAS utilizes **vDEVs (Virtual Devices)** as building blocks for pools, and understanding how to plan and size them is crucial for performance and data safety. Learn more about the considerations for **special vDEVs (SVDEVs)** and how they can impact your setup:

🔗 **[Read more about vDEVs](https://forums.truenas.com/t/special-vdev-svdev-planning-sizing-and-considerations/5086)**

# TrueNAS Notes

## Bonding Two Network Interfaces for Improved Performance  
[TrueNAS LAGG Setup Guide](https://www.truenas.com/docs/scale/scaletutorials/network/interfaces/settinguplagg/)  

- Attempted to bond two network interfaces for improved performance.  
- The Unifi Pro 24 supports aggregation, but the TrueNAS test failed.  
- Need to investigate further why the bond is not working initially.  

---

## Extending VDEVs in New TrueNAS SCALE  

- Used the new VDEV extension feature.  
- After a day, it was stuck at **25%**, so I manually restarted TrueNAS.  
- The four drives appeared, but the capacity was incorrect.  
- Ran the following command to check status:  

  ```sh
  sudo zpool status -v
  ```

- Since **6 days ago**, the expansion has been running.  
- Will wait a bit longer to see if it completes.
