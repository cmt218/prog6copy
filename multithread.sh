echo "begin multithread script"
./obj64/Client -senceladus -p9003 -Ptest.txt &
./obj64/Client -senceladus -p9003 -P./tempdir/test.txt &
./obj64/Client -senceladus -p9003 -Ptest.txt &
./obj64/Client -senceladus -p9003 -P./tempdir/test.txt &
./obj64/Client -senceladus -p9003 -Ptest.txt &
./obj64/Client -senceladus -p9003 -P./tempdir/test.txt &
./obj64/Client -senceladus -p9003 -Ptest.txt &
./obj64/Client -senceladus -p9003 -P./tempdir/test.txt &
./obj64/Client -senceladus -p9003 -Ptest.txt &
./obj64/Client -senceladus -p9003 -P./tempdir/test.txt &
wait
echo "end multithread script"