killall receiver_manage; killall sender_manager; killall hackler_manager;
make;
./sender_manager InputFiles/F0.csv & tee logs/sender.txt & ./receiver_manager & tee logs/receiver.txt & ./hackler InputFiles/F7.csv & tee logs/hackler.txt;
