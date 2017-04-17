set -e
sudo rmmod macusbfb || true
sudo rmmod fb || true
sudo rm -f /lib/modules/2.6.34/extra/macusbfb.ko
sudo make clean install
sudo modprobe macusbfb
sleep 2
#echo bla | sudo tee /dev/fb0
#sudo Xfbdev :1
sudo test/test
sudo modprobe snd-usb-audio
mplayer -vo fbdev -zoom -fs -vf scale  ~/server/film/Bastian\ -\ You\ got\ my\ love.avi  -ao alsa:device=hw=1
dmesg | tail