
image nanopi fire 2a :  s5p4418-sd-lubuntu-desktop-xenial-4.4-armhf-20230825.img
(https://dl.friendlyelec.com/nanopifire2a)


// etc/network/interfaces 
auto lo
iface lo inet loopback

auto eth0

iface eth0 inet static
address 192.168.2.100
netmask 255.255.255.0
gateway 192.168.2.254

#auto wlxe84e066dc671
#iface wlxe84e066dc671 inet static
#address 192.168.2.131
#netmask 255.255.255.0
#gateway 192.168.2.254
#nameserver 192.168.2.254
#nameserver 8.8.8.8
#nameserver 127.0.0.53


dns-search lan

// gui uit:
systemctl disable lightdm 
service lightdm stop

Audio:
cat /proc/asound/cards
etc/asound.conf:
pcm.!default {
    type hw
    card 2
}

ctl.!default {
    type hw
    card 2
}

camera shellbestand in init.d zetten en excutable maken.
link in rc5.d: ln -s /etc/init.d/camera /etc/rc5.d/S99camera

