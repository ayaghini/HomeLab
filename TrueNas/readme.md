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
