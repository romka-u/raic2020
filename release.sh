rm -f submit.zip
mv CMakeLists.{txt,bckp}
cp CMakeLists.{sub,txt}
zip submit.zip -r model/* *.h *.{c,h}pp CMakeLists.txt 2>&1 1>/dev/null
mv CMakeLists.{bckp,txt}

wc -l *.h

for i in $(seq -f "%04g" 1 1991); do
    fs="submits/submit${i}.zip"
    if [ ! -f "$fs" ]; then
        echo "Copy submit.zip to $fs and binary"
        cp submit.zip $fs
        cp build/aicup2020 build/aicup2020-v${i}
        break
    fi
done
