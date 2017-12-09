echo "begin multithread script"
./obj64/Client -scaliban -p9003 -Ptest.txt -a &
./obj64/Client -scaliban -p9003 -Gtest.txt -a &
./obj64/Client -scaliban -p9003 -Ptest.txt -a &
./obj64/Client -scaliban -p9003 -Gtest.txt -a &
./obj64/Client -scaliban -p9003 -Ptest.txt -a &
./obj64/Client -scaliban -p9003 -Gtest.txt -a &
./obj64/Client -scaliban -p9003 -Ptest.txt -a &
./obj64/Client -scaliban -p9003 -Gtest.txt -a &
wait
echo "end multithread script"