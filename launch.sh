killall receiver_manage; killall sender_manager; killall hackler_manager;
make;
./sender_manager InputFiles/F0.csv & ./receiver_manager & ./hackler InputFiles/F7.csv > logs;
