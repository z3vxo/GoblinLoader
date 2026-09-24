rm -rf ~/.local/share/ldr/ 
sudo ./server setup
./server register test test http://127.0.0.1:8080
./server run --addr 0.0.0.0 --port 8081
