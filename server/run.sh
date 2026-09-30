rm -rf ~/.local/share/ldr/ 
sudo ./server setup
./server register test test http://127.0.0.1:8080
python3 seed.py
cp -r ../modules/output/* ~/.local/share/ldr/modules/
cp ../exemap/loader.bin ~/.local/share/ldr/utils/
