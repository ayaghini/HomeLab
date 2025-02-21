# Setting Up an Older Label Maker on a Main Server

I wanted to move my old label maker to the main server so I could use it from anywhere. Initially, I tried using ChatGPT to find the right driver, but it failed. So, this setup guide is a mix of **Google searches and GPT-assisted troubleshooting**—haha! 😄

## Installation and Configuration Steps

### 1. Install CUPS (Common Unix Printing System)
```sh
sudo apt install cups
```

### 2. Configure CUPS to Allow Remote Access
Edit the CUPS configuration file:
```sh
sudo nano /etc/cups/cupsd.conf
```
Modify/add the following lines:
```
Listen 0.0.0.0:631
Order allow,deny
Allow all
```
Save and exit (`CTRL + X`, then `Y`, then `Enter`).

### 3. Install the Printer Driver
For Brother label printers (P-Touch series), install the necessary driver:
```sh
sudo apt install printer-driver-ptouch
```

### 4. Restart and Enable CUPS
```sh
sudo systemctl restart cups
sudo systemctl enable cups
```

### 5. Access the CUPS Web Interface
- Open your browser and go to:  
  **`http://<rpi-ip>:631`** (Replace `<rpi-ip>` with your Raspberry Pi’s IP address)
- Navigate to the **Administration** tab.
- Click **Add Printer** and follow the setup instructions.

---

Now, the label maker should be accessible from anywhere on your network! 🎉  
Hope this helps anyone else trying to set up an older printer remotely. 🚀
